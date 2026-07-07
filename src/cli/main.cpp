#include <cstdlib>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "clot/codegen/llvm_compiler.hpp"
#include "clot/frontend/parser.hpp"
#include "clot/frontend/source_loader.hpp"
#include "clot/frontend/static_analyzer.hpp"
#include "clot/interpreter/interpreter.hpp"
#include "clot/runtime/env.hpp"
#include "clot/runtime/i18n.hpp"
#include "clot/runtime/paths.hpp"

#ifndef CLOT_VERSION
#define CLOT_VERSION "0.3.3"
#endif

namespace {

enum class RunMode {
    Interpret,
    Compile,
    Analyze,
};

struct CliOptions {
    bool show_help = false;
    bool show_version = false;
    bool verbose = false;
    std::string input_path;
    RunMode mode = RunMode::Interpret;
    clot::runtime::Language language = clot::runtime::Language::English;
    clot::codegen::CompileOptions compile_options;
};

void PrintVersion() {
    std::cout << "clot " << CLOT_VERSION << "\n";
}

void PrintHelp() {
    if (clot::runtime::GetLanguage() == clot::runtime::Language::English) {
        std::cout
            << "ClotProgrammingLanguage\n"
            << "Usage:\n"
            << "  clot [file.clot] [options]\n\n"
            << "Options:\n"
            << "  -h, --help               Show this help\n"
            << "  -v, --version            Show the clot version\n"
            << "  --mode interpret|compile|analyze Run in interpreter, LLVM compiler or static analyzer mode\n"
            << "  --emit exe|obj|ir        Output type in compile mode\n"
            << "  -o, --output <file>      Output path in compile mode\n"
            << "  --target <triple>        LLVM target (e.g. x86_64-pc-linux-gnu)\n"
            << "  --runtime-bridge static|external Runtime bridge strategy in compile mode\n"
            << "  --lang es|en             UI language (Spanish/English)\n"
            << "  --verbose                Print extra information\n\n"
            << "Examples:\n"
            << "  clot program.clot\n"
            << "  clot program.clot --mode compile --emit exe -o program\n"
            << "  clot program.clot --mode analyze\n"
            << "  clot program.clot --mode compile --emit ir -o program.ll\n";
        return;
    }

    std::cout
        << "ClotProgrammingLanguage\n"
        << "Uso:\n"
        << "  clot [archivo.clot] [opciones]\n\n"
        << "Opciones:\n"
        << "  -h, --help               Muestra esta ayuda\n"
        << "  -v, --version            Muestra la version de clot\n"
        << "  --mode interpret|compile|analyze Ejecuta en modo interprete, compilador LLVM o analizador estatico\n"
        << "  --emit exe|obj|ir        Tipo de salida en modo compile\n"
        << "  -o, --output <archivo>   Ruta de salida en modo compile\n"
        << "  --target <triple>        Target LLVM (ej. x86_64-pc-linux-gnu)\n"
        << "  --runtime-bridge static|external Estrategia del runtime bridge en compile\n"
        << "  --lang es|en             Idioma de interfaz\n"
        << "  --verbose                Imprime informacion adicional\n\n"
        << "Ejemplos:\n"
        << "  clot programa.clot\n"
        << "  clot programa.clot --mode compile --emit exe -o programa\n"
        << "  clot programa.clot --mode analyze\n"
        << "  clot programa.clot --mode compile --emit ir -o programa.ll\n";
}

std::string FindDefaultInput() {
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path())) {
        if (entry.is_regular_file() && entry.path().extension() == ".clot") {
            return entry.path().string();
        }
    }

    const std::filesystem::path root_test = "test.clot";
    if (std::filesystem::exists(root_test)) {
        return root_test.string();
    }

    const std::filesystem::path example_test = std::filesystem::path("examples") / "basic.clot";
    if (std::filesystem::exists(example_test)) {
        return example_test.string();
    }

    return {};
}

std::string BuildDefaultOutput(
    const std::string& input_path,
    clot::codegen::CompileOptions::EmitKind emit_kind) {
    std::filesystem::path base = std::filesystem::path(input_path).stem();
    if (base.empty()) {
        base = "clot_output";
    }

    if (emit_kind == clot::codegen::CompileOptions::EmitKind::IR) {
        return base.string() + ".ll";
    }

    if (emit_kind == clot::codegen::CompileOptions::EmitKind::Object) {
        return base.string() + ".o";
    }

#ifdef _WIN32
    return base.string() + ".exe";
#else
    return base.string();
#endif
}

bool IsUnhandledExceptionDiagnostic(const std::string& diagnostic) {
    return diagnostic.rfind("Excepcion no capturada: ", 0) == 0 ||
           diagnostic.rfind("Unhandled Exception: ", 0) == 0;
}

bool StartsWithPrefix(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

std::filesystem::path WithClotExtension(std::filesystem::path path) {
    if (path.extension().empty()) {
        path += ".clot";
    }
    return path;
}

void AddUniqueCandidate(std::vector<std::filesystem::path>* candidates, const std::filesystem::path& candidate) {
    if (candidates == nullptr) {
        return;
    }
    for (const auto& existing : *candidates) {
        if (existing == candidate) {
            return;
        }
    }
    candidates->push_back(candidate);
}

std::filesystem::path DotPathToFolderPath(std::string module_name) {
    const auto separator =
        static_cast<std::string::value_type>(std::filesystem::path::preferred_separator);
    std::replace(module_name.begin(), module_name.end(), '.', separator);
    return std::filesystem::path(module_name);
}

std::vector<std::filesystem::path> DotPathToRelativeCandidates(std::string module_name) {
    const std::filesystem::path folder_path = DotPathToFolderPath(std::move(module_name));
    std::vector<std::filesystem::path> candidates;

    AddUniqueCandidate(&candidates, WithClotExtension(folder_path));
    if (folder_path.has_filename()) {
        AddUniqueCandidate(&candidates, folder_path / WithClotExtension(folder_path.filename()));
    }
    return candidates;
}

void AddCandidatesWithRoot(std::vector<std::filesystem::path>* candidates,
                           const std::filesystem::path& root,
                           const std::string& module_name) {
    for (const auto& relative_candidate : DotPathToRelativeCandidates(module_name)) {
        AddUniqueCandidate(candidates, root / relative_candidate);
    }
}

void AddCandidatesWithPrefixedRoot(std::vector<std::filesystem::path>* candidates,
                                   const std::filesystem::path& root,
                                   const std::string& module_name,
                                   const std::string& prefix) {
    if (!StartsWithPrefix(module_name, prefix)) {
        return;
    }
    AddCandidatesWithRoot(candidates, root, module_name.substr(prefix.size()));
}

std::vector<std::filesystem::path> CollectAncestorRoots(const std::filesystem::path& start) {
    std::vector<std::filesystem::path> roots;
    if (start.empty()) {
        return roots;
    }

    std::filesystem::path current = start;
    if (current.is_relative()) {
        current = std::filesystem::current_path() / current;
    }
    current = current.lexically_normal();
    while (true) {
        AddUniqueCandidate(&roots, current);
        const std::filesystem::path parent = current.parent_path();
        if (parent.empty() || parent == current) {
            break;
        }
        current = parent;
    }
    return roots;
}

std::filesystem::path ResolveModulePathForAnalyze(const std::string& module_name,
                                                  const std::filesystem::path& current_module_dir) {
    std::vector<std::filesystem::path> search_roots = CollectAncestorRoots(current_module_dir);
    // Also consult the installed standard library (binary-relative / CLOT_HOME)
    // so `analyze` resolves stdlib imports the same way the interpreter does.
    for (const auto& install_root : clot::runtime::StdlibSearchRoots()) {
        AddUniqueCandidate(&search_roots, install_root);
    }
    std::vector<std::filesystem::path> candidates;
    const auto relative_candidates = DotPathToRelativeCandidates(module_name);

    for (const auto& relative_candidate : relative_candidates) {
        AddUniqueCandidate(&candidates, current_module_dir / relative_candidate);
    }

    for (const auto& root : search_roots) {
        AddCandidatesWithRoot(&candidates, root, module_name);
    }

    for (const auto& root : search_roots) {
        AddCandidatesWithPrefixedRoot(&candidates, root / "clot" / "science", module_name, "modules.");
        AddCandidatesWithPrefixedRoot(&candidates, root / "clot" / "core", module_name, "mods.");

        AddCandidatesWithRoot(&candidates, root / "clot", module_name);
        AddCandidatesWithPrefixedRoot(&candidates, root / "clot", module_name, "clot.");
        AddCandidatesWithRoot(&candidates, root / "clot" / "science", module_name);
        AddCandidatesWithPrefixedRoot(&candidates, root / "clot" / "science", module_name, "science.");
        AddCandidatesWithRoot(&candidates, root / "clot" / "core", module_name);
        AddCandidatesWithPrefixedRoot(&candidates, root / "clot" / "core", module_name, "core.");
        AddCandidatesWithRoot(&candidates, root / "clot" / "io", module_name);
        AddCandidatesWithPrefixedRoot(&candidates, root / "clot" / "io", module_name, "io.");
        AddCandidatesWithRoot(&candidates, root / "clot" / "ml", module_name);
        AddCandidatesWithPrefixedRoot(&candidates, root / "clot" / "ml", module_name, "ml.");

        AddCandidatesWithRoot(&candidates, root / "modules", module_name);
        AddCandidatesWithRoot(&candidates, root / "mods", module_name);
        AddCandidatesWithPrefixedRoot(&candidates, root / "modules", module_name, "modules.");
        AddCandidatesWithPrefixedRoot(&candidates, root / "mods", module_name, "mods.");
    }

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return candidates.empty()
               ? current_module_dir / WithClotExtension(DotPathToFolderPath(module_name))
               : candidates.front();
}

bool ParseProgramFromFile(const std::filesystem::path& file_path,
                          std::unique_ptr<clot::frontend::Program>* out_program,
                          std::string* out_error) {
    if (out_program == nullptr || out_error == nullptr) {
        return false;
    }

    std::vector<std::string> lines;
    std::string load_error;
    if (!clot::frontend::LoadSourceLines(file_path.string(), &lines, &load_error)) {
        *out_error = "Error importando modulo '" + file_path.string() + "': " + load_error;
        return false;
    }

    auto program = std::make_unique<clot::frontend::Program>();
    clot::frontend::Parser parser(std::move(lines));
    clot::frontend::Diagnostic diagnostic;
    if (!parser.Parse(program.get(), &diagnostic)) {
        *out_error = "Error de parseo importando modulo '" + file_path.string() + "' en linea " +
                     std::to_string(diagnostic.line) + ", columna " + std::to_string(diagnostic.column) +
                     ": " + diagnostic.message;
        return false;
    }

    *out_program = std::move(program);
    return true;
}

void CollectImportsRecursive(const std::vector<std::unique_ptr<clot::frontend::Statement>>& statements,
                            std::vector<const clot::frontend::ImportStmt*>* out_imports) {
    if (out_imports == nullptr) {
        return;
    }

    for (const auto& statement : statements) {
        if (statement == nullptr) {
            continue;
        }

        if (const auto* import_stmt = dynamic_cast<const clot::frontend::ImportStmt*>(statement.get())) {
            out_imports->push_back(import_stmt);
            continue;
        }

        if (const auto* function_decl = dynamic_cast<const clot::frontend::FunctionDeclStmt*>(statement.get())) {
            CollectImportsRecursive(function_decl->body, out_imports);
            continue;
        }

        if (const auto* conditional = dynamic_cast<const clot::frontend::IfStmt*>(statement.get())) {
            CollectImportsRecursive(conditional->then_branch, out_imports);
            CollectImportsRecursive(conditional->else_branch, out_imports);
            continue;
        }

        if (const auto* while_stmt = dynamic_cast<const clot::frontend::WhileStmt*>(statement.get())) {
            CollectImportsRecursive(while_stmt->body, out_imports);
            continue;
        }

        if (const auto* for_stmt = dynamic_cast<const clot::frontend::ForStmt*>(statement.get())) {
            CollectImportsRecursive(for_stmt->body, out_imports);
            continue;
        }

        if (const auto* foreach_stmt = dynamic_cast<const clot::frontend::ForEachStmt*>(statement.get())) {
            CollectImportsRecursive(foreach_stmt->body, out_imports);
            continue;
        }

        if (const auto* do_while_stmt = dynamic_cast<const clot::frontend::DoWhileStmt*>(statement.get())) {
            CollectImportsRecursive(do_while_stmt->body, out_imports);
            continue;
        }

        if (const auto* switch_stmt = dynamic_cast<const clot::frontend::SwitchStmt*>(statement.get())) {
            for (const auto& switch_case : switch_stmt->cases) {
                CollectImportsRecursive(switch_case.body, out_imports);
            }
            continue;
        }

        if (const auto* try_catch_stmt = dynamic_cast<const clot::frontend::TryCatchStmt*>(statement.get())) {
            CollectImportsRecursive(try_catch_stmt->try_branch, out_imports);
            CollectImportsRecursive(try_catch_stmt->catch_branch, out_imports);
            CollectImportsRecursive(try_catch_stmt->finally_branch, out_imports);
            continue;
        }

        if (const auto* class_decl = dynamic_cast<const clot::frontend::ClassDeclStmt*>(statement.get())) {
            CollectImportsRecursive(class_decl->constructor_body, out_imports);
            for (const auto& method : class_decl->methods) {
                CollectImportsRecursive(method.body, out_imports);
            }
            for (const auto& accessor : class_decl->accessors) {
                CollectImportsRecursive(accessor.body, out_imports);
            }
            continue;
        }
    }
}

bool LoadModuleForAnalyze(const std::filesystem::path& module_path,
                         std::set<std::string>* loaded,
                         std::set<std::string>* loading,
                         std::vector<std::unique_ptr<clot::frontend::Program>>* out_programs,
                         std::string* out_error) {
    if (loaded == nullptr || loading == nullptr || out_programs == nullptr || out_error == nullptr) {
        return false;
    }

    std::error_code ec;
    const std::string module_id =
        std::filesystem::weakly_canonical(module_path, ec).string();
    const std::string normalized_id = ec ? module_path.lexically_normal().string() : module_id;

    if (loaded->count(normalized_id) > 0) {
        return true;
    }
    if (loading->count(normalized_id) > 0) {
        *out_error = "Import circular detectado en modulo: " + normalized_id;
        return false;
    }

    loading->insert(normalized_id);

    std::unique_ptr<clot::frontend::Program> program;
    if (!ParseProgramFromFile(module_path, &program, out_error)) {
        loading->erase(normalized_id);
        return false;
    }

    std::vector<const clot::frontend::ImportStmt*> imports;
    CollectImportsRecursive(program->statements, &imports);
    for (const auto* import_stmt : imports) {
        if (import_stmt == nullptr || import_stmt->module_name.empty() || import_stmt->module_name == "math") {
            continue;
        }

        const std::filesystem::path imported_path =
            ResolveModulePathForAnalyze(import_stmt->module_name, module_path.parent_path());
        if (!std::filesystem::exists(imported_path)) {
            *out_error = "No se encontro modulo importado '" + import_stmt->module_name +
                         "' (resuelto como '" + imported_path.string() + "').";
            loading->erase(normalized_id);
            return false;
        }

        if (!LoadModuleForAnalyze(imported_path, loaded, loading, out_programs, out_error)) {
            loading->erase(normalized_id);
            return false;
        }
    }

    loading->erase(normalized_id);
    loaded->insert(normalized_id);
    out_programs->push_back(std::move(program));
    return true;
}

bool BuildAnalyzeProgramSet(const std::string& entry_path,
                            const clot::frontend::Program& entry_program,
                            std::vector<std::unique_ptr<clot::frontend::Program>>* out_imported_programs,
                            std::string* out_error) {
    if (out_imported_programs == nullptr || out_error == nullptr) {
        return false;
    }
    out_imported_programs->clear();

    std::vector<const clot::frontend::ImportStmt*> imports;
    CollectImportsRecursive(entry_program.statements, &imports);

    std::set<std::string> loaded;
    std::set<std::string> loading;
    const std::filesystem::path entry_file_path(entry_path);
    for (const auto* import_stmt : imports) {
        if (import_stmt == nullptr || import_stmt->module_name.empty() || import_stmt->module_name == "math") {
            continue;
        }

        const std::filesystem::path imported_path =
            ResolveModulePathForAnalyze(import_stmt->module_name, entry_file_path.parent_path());
        if (!std::filesystem::exists(imported_path)) {
            *out_error = "No se encontro modulo importado '" + import_stmt->module_name +
                         "' (resuelto como '" + imported_path.string() + "').";
            return false;
        }

        if (!LoadModuleForAnalyze(imported_path, &loaded, &loading, out_imported_programs, out_error)) {
            return false;
        }
    }

    return true;
}

bool ParseArgs(int argc, char* argv[], CliOptions* out_options, std::string* out_error) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            out_options->show_help = true;
            continue;
        }

        if (arg == "-v" || arg == "--version") {
            out_options->show_version = true;
            continue;
        }

        if (arg == "--verbose") {
            out_options->verbose = true;
            out_options->compile_options.verbose = true;
            continue;
        }

        if (arg == "--mode") {
            if (i + 1 >= argc) {
                *out_error = clot::runtime::Tr("Falta valor para --mode.", "Missing value for --mode.");
                return false;
            }

            const std::string value = argv[++i];
            if (value == "interpret") {
                out_options->mode = RunMode::Interpret;
            } else if (value == "compile") {
                out_options->mode = RunMode::Compile;
            } else if (value == "analyze") {
                out_options->mode = RunMode::Analyze;
            } else {
                *out_error = clot::runtime::Tr("Modo invalido: ", "Invalid mode: ") + value;
                return false;
            }
            continue;
        }

        if (arg == "--emit") {
            if (i + 1 >= argc) {
                *out_error = clot::runtime::Tr("Falta valor para --emit.", "Missing value for --emit.");
                return false;
            }

            const std::string value = argv[++i];
            if (value == "exe") {
                out_options->compile_options.emit_kind = clot::codegen::CompileOptions::EmitKind::Executable;
            } else if (value == "obj") {
                out_options->compile_options.emit_kind = clot::codegen::CompileOptions::EmitKind::Object;
            } else if (value == "ir") {
                out_options->compile_options.emit_kind = clot::codegen::CompileOptions::EmitKind::IR;
            } else {
                *out_error = clot::runtime::Tr("Emit invalido: ", "Invalid emit kind: ") + value;
                return false;
            }
            continue;
        }

        if (arg == "--lang") {
            if (i + 1 >= argc) {
                *out_error = clot::runtime::Tr("Falta valor para --lang.", "Missing value for --lang.");
                return false;
            }

            const std::string value = argv[++i];
            clot::runtime::Language parsed_language = clot::runtime::Language::Spanish;
            if (!clot::runtime::ParseLanguage(value, &parsed_language)) {
                *out_error = clot::runtime::Tr(
                    "Idioma invalido. Use es o en.",
                    "Invalid language. Use es or en.");
                return false;
            }

            out_options->language = parsed_language;
            clot::runtime::SetLanguage(parsed_language);
            continue;
        }

        if (arg == "-o" || arg == "--output") {
            if (i + 1 >= argc) {
                *out_error = clot::runtime::Tr("Falta valor para --output.", "Missing value for --output.");
                return false;
            }
            out_options->compile_options.output_path = argv[++i];
            continue;
        }

        if (arg == "--target") {
            if (i + 1 >= argc) {
                *out_error = clot::runtime::Tr("Falta valor para --target.", "Missing value for --target.");
                return false;
            }
            out_options->compile_options.target_triple = argv[++i];
            continue;
        }

        if (arg == "--runtime-bridge") {
            if (i + 1 >= argc) {
                *out_error = clot::runtime::Tr(
                    "Falta valor para --runtime-bridge.",
                    "Missing value for --runtime-bridge.");
                return false;
            }

            const std::string value = argv[++i];
            if (value == "static") {
                out_options->compile_options.runtime_bridge_mode =
                    clot::codegen::CompileOptions::RuntimeBridgeMode::Static;
            } else if (value == "external") {
                out_options->compile_options.runtime_bridge_mode =
                    clot::codegen::CompileOptions::RuntimeBridgeMode::External;
            } else {
                *out_error = clot::runtime::Tr(
                    "Runtime bridge invalido. Use static o external.",
                    "Invalid runtime bridge. Use static or external.");
                return false;
            }
            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            *out_error = clot::runtime::Tr("Opcion desconocida: ", "Unknown option: ") + arg;
            return false;
        }

        if (out_options->input_path.empty()) {
            out_options->input_path = arg;
        } else {
            *out_error = "Se recibieron multiples archivos de entrada.";
            return false;
        }
    }

    if (out_options->input_path.empty() && !out_options->show_help && !out_options->show_version) {
        out_options->input_path = FindDefaultInput();
    }

    if (out_options->input_path.empty() && !out_options->show_help && !out_options->show_version) {
        *out_error = clot::runtime::Tr(
            "No se encontro archivo .clot de entrada.",
            "No input .clot file was found.");
        return false;
    }

    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    CliOptions options;

    if (const auto env_lang = clot::runtime::GetEnvVar("CLOT_LANG"); env_lang) {
        clot::runtime::Language parsed_language = clot::runtime::Language::Spanish;
        if (clot::runtime::ParseLanguage(*env_lang, &parsed_language)) {
            options.language = parsed_language;
            clot::runtime::SetLanguage(parsed_language);
        }
    }

    std::string cli_error;
    if (!ParseArgs(argc, argv, &options, &cli_error)) {
        std::cerr << clot::runtime::Tr("Error: ", "Error: ")
                  << clot::runtime::TranslateDiagnostic(cli_error) << "\n";
        std::cerr << clot::runtime::Tr(
                         "Use --help para ver las opciones disponibles.\n",
                         "Use --help to see available options.\n");
        return 1;
    }

    clot::runtime::SetLanguage(options.language);

    if (options.show_version) {
        PrintVersion();
        return 0;
    }

    if (options.show_help) {
        PrintHelp();
        return 0;
    }

    std::vector<std::string> lines;
    std::string load_error;
    if (!clot::frontend::LoadSourceLines(options.input_path, &lines, &load_error)) {
        std::cerr << clot::runtime::Tr("Error: ", "Error: ")
                  << clot::runtime::TranslateDiagnostic(load_error) << "\n";
        return 1;
    }

    std::string source_text;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        source_text += lines[i];
        if (i + 1 < lines.size()) {
            source_text.push_back('\n');
        }
    }

    clot::frontend::Parser parser(std::move(lines));
    clot::frontend::Program program;
    clot::frontend::Diagnostic diagnostic;
    if (!parser.Parse(&program, &diagnostic)) {
        std::string diagnostic_message = clot::runtime::TranslateDiagnostic(diagnostic.message);
        std::cerr << clot::runtime::Tr("Error de parseo en linea ", "Parse error at line ")
                  << diagnostic.line
                  << clot::runtime::Tr(", columna ", ", column ")
                  << diagnostic.column
                  << ": "
                  << diagnostic_message
                  << "\n";
        return 1;
    }

    if (options.mode == RunMode::Analyze) {
        std::vector<std::unique_ptr<clot::frontend::Program>> imported_programs;
        std::string import_resolution_error;
        if (!BuildAnalyzeProgramSet(options.input_path, program, &imported_programs, &import_resolution_error)) {
            std::cerr << clot::runtime::Tr("Error: ", "Error: ")
                      << clot::runtime::TranslateDiagnostic(import_resolution_error) << "\n";
            return 1;
        }

        std::vector<const clot::frontend::Program*> analysis_units;
        analysis_units.reserve(imported_programs.size() + 1);
        for (const auto& imported_program : imported_programs) {
            analysis_units.push_back(imported_program.get());
        }
        analysis_units.push_back(&program);

        clot::frontend::StaticAnalyzer analyzer;
        clot::frontend::AnalysisReport report;
        analyzer.Analyze(analysis_units, &report);

        for (const auto& warning : report.warnings) {
            std::cerr << clot::runtime::Tr("Advertencia: ", "Warning: ")
                      << clot::runtime::TranslateDiagnostic(warning.message) << "\n";
        }

        for (const auto& error : report.errors) {
            std::cerr << clot::runtime::Tr("Error: ", "Error: ")
                      << clot::runtime::TranslateDiagnostic(error.message) << "\n";
        }

        if (report.errors.empty()) {
            if (options.verbose) {
                std::cout << clot::runtime::Tr(
                                 "Analisis estatico sin errores criticos.\n",
                                 "Static analysis completed with no critical errors.\n");
            }
            return 0;
        }

        return 1;
    }

    if (options.mode == RunMode::Interpret) {
        clot::interpreter::Interpreter interpreter;
        interpreter.SetEntryFilePath(options.input_path);
        std::string runtime_error;
        if (!interpreter.Execute(program, &runtime_error)) {
            const std::string translated_runtime_error = clot::runtime::TranslateDiagnostic(runtime_error);
            if (IsUnhandledExceptionDiagnostic(translated_runtime_error)) {
                std::cerr << translated_runtime_error << "\n";
            } else {
                std::cerr << clot::runtime::Tr("Error de ejecucion: ", "Runtime error: ")
                          << translated_runtime_error << "\n";
            }
            return 1;
        }

        return 0;
    }

    if (!clot::codegen::LlvmCompiler::IsAvailable()) {
        std::cerr
            << clot::runtime::Tr(
                   "Error: este binario no tiene soporte LLVM habilitado.\n",
                   "Error: this binary does not have LLVM support enabled.\n")
            << clot::runtime::Tr(
                   "Instala LLVM en WSL y recompila con CMake (scripts/install_llvm_wsl.sh).\n",
                   "Install LLVM in WSL and rebuild with CMake (scripts/install_llvm_wsl.sh).\n");
        return 1;
    }

    if (options.compile_options.output_path.empty()) {
        options.compile_options.output_path = BuildDefaultOutput(options.input_path, options.compile_options.emit_kind);
    }

    options.compile_options.input_path = options.input_path;
    options.compile_options.source_text = source_text;
    options.compile_options.project_root = std::filesystem::current_path().string();

    clot::codegen::LlvmCompiler compiler;
    std::string compile_error;
    if (!compiler.Compile(program, options.compile_options, &compile_error)) {
        std::cerr << clot::runtime::Tr("Error de compilacion LLVM: ", "LLVM compilation error: ")
                  << clot::runtime::TranslateDiagnostic(compile_error) << "\n";
        return 1;
    }

    if (options.verbose) {
        std::cout << clot::runtime::Tr("Salida generada: ", "Generated output: ")
                  << options.compile_options.output_path << "\n";
    }

    return 0;
}
