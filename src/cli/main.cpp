#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "clot/codegen/llvm_compiler.hpp"
#include "clot/frontend/parser.hpp"
#include "clot/frontend/source_loader.hpp"
#include "clot/interpreter/interpreter.hpp"
#include "clot/runtime/i18n.hpp"

namespace {

enum class RunMode {
    Interpret,
    Compile,
};

struct CliOptions {
    bool show_help = false;
    bool verbose = false;
    std::string input_path;
    RunMode mode = RunMode::Interpret;
    clot::runtime::Language language = clot::runtime::Language::Spanish;
    clot::codegen::CompileOptions compile_options;
};

void PrintHelp() {
    if (clot::runtime::GetLanguage() == clot::runtime::Language::English) {
        std::cout
            << "ClotProgrammingLanguage\n"
            << "Usage:\n"
            << "  clot [file.clot] [options]\n\n"
            << "Options:\n"
            << "  -h, --help               Show this help\n"
            << "  --mode interpret|compile Run in interpreter or LLVM compiler mode\n"
            << "  --emit exe|obj|ir        Output type in compile mode\n"
            << "  -o, --output <file>      Output path in compile mode\n"
            << "  --target <triple>        LLVM target (e.g. x86_64-pc-linux-gnu)\n"
            << "  --lang es|en             UI language (Spanish/English)\n"
            << "  --verbose                Print extra information\n\n"
            << "Examples:\n"
            << "  clot program.clot\n"
            << "  clot program.clot --mode compile --emit exe -o program\n"
            << "  clot program.clot --mode compile --emit ir -o program.ll\n";
        return;
    }

    std::cout
        << "ClotProgrammingLanguage\n"
        << "Uso:\n"
        << "  clot [archivo.clot] [opciones]\n\n"
        << "Opciones:\n"
        << "  -h, --help               Muestra esta ayuda\n"
        << "  --mode interpret|compile Ejecuta en modo interprete o compilador LLVM\n"
        << "  --emit exe|obj|ir        Tipo de salida en modo compile\n"
        << "  -o, --output <archivo>   Ruta de salida en modo compile\n"
        << "  --target <triple>        Target LLVM (ej. x86_64-pc-linux-gnu)\n"
        << "  --lang es|en             Idioma de interfaz\n"
        << "  --verbose                Imprime informacion adicional\n\n"
        << "Ejemplos:\n"
        << "  clot programa.clot\n"
        << "  clot programa.clot --mode compile --emit exe -o programa\n"
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

bool ParseArgs(int argc, char* argv[], CliOptions* out_options, std::string* out_error) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            out_options->show_help = true;
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

    if (out_options->input_path.empty() && !out_options->show_help) {
        out_options->input_path = FindDefaultInput();
    }

    if (out_options->input_path.empty() && !out_options->show_help) {
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

    if (const char* env_lang = std::getenv("CLOT_LANG"); env_lang != nullptr) {
        clot::runtime::Language parsed_language = clot::runtime::Language::Spanish;
        if (clot::runtime::ParseLanguage(env_lang, &parsed_language)) {
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

    if (options.mode == RunMode::Interpret) {
        clot::interpreter::Interpreter interpreter;
        interpreter.SetEntryFilePath(options.input_path);
        std::string runtime_error;
        if (!interpreter.Execute(program, &runtime_error)) {
            std::cerr << clot::runtime::Tr("Error de ejecucion: ", "Runtime error: ")
                      << clot::runtime::TranslateDiagnostic(runtime_error) << "\n";
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
