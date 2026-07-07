#include "clot/interpreter/interpreter.hpp"

#include <algorithm>
#include <filesystem>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "clot/frontend/parser.hpp"
#include "clot/frontend/source_loader.hpp"
#include "clot/runtime/paths.hpp"

namespace clot::interpreter {

namespace {

bool StartsWithPrefix(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

// Namespace handle for a dotted module name: `scripts.math` -> `math`,
// `inventario` -> `inventario`. Used so a plain `import mod;` exposes the module
// as a dot-access object, symmetric with `import mod as alias;`.
std::string LastDottedSegment(const std::string& module_name) {
    const std::size_t separator = module_name.find_last_of('.');
    if (separator == std::string::npos) {
        return module_name;
    }
    return module_name.substr(separator + 1);
}

std::filesystem::path WithClotExtension(std::filesystem::path path) {
    if (path.extension().empty()) {
        path += ".clot";
    }
    return path;
}

void AddUniqueCandidate(
    std::vector<std::filesystem::path>* candidates,
    const std::filesystem::path& candidate) {
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

    // Canonical dotted import path: a.b.c -> a/b/c.clot
    AddUniqueCandidate(&candidates, WithClotExtension(folder_path));
    if (folder_path.has_filename()) {
        // Folder module path: a.b.c -> a/b/c/c.clot
        AddUniqueCandidate(&candidates, folder_path / WithClotExtension(folder_path.filename()));
    }
    return candidates;
}

void AddCandidatesWithRoot(
    std::vector<std::filesystem::path>* candidates,
    const std::filesystem::path& root,
    const std::string& module_name) {
    for (const auto& relative_candidate : DotPathToRelativeCandidates(module_name)) {
        AddUniqueCandidate(candidates, root / relative_candidate);
    }
}

void AddCandidatesWithPrefixedRoot(
    std::vector<std::filesystem::path>* candidates,
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

}  // namespace

bool Interpreter::ImportModule(const std::string& module_name, std::string* out_module_id, std::string* out_error) {
    if (module_name == "math") {
        imported_modules_.insert(module_name);
        if (out_module_id != nullptr) {
            *out_module_id = module_name;
        }
        return true;
    }

    const std::filesystem::path module_path = ResolveModulePath(module_name);
    std::error_code ec;
    const std::string module_id = std::filesystem::weakly_canonical(module_path, ec).string();
    const std::string normalized_module_id = ec ? module_path.lexically_normal().string() : module_id;

    if (out_module_id != nullptr) {
        *out_module_id = normalized_module_id;
    }

    if (imported_modules_.count(normalized_module_id) > 0) {
        return true;
    }

    if (importing_modules_.count(normalized_module_id) > 0) {
        *out_error = "Import circular detectado en modulo: " + normalized_module_id;
        return false;
    }

    importing_modules_.insert(normalized_module_id);
    ModuleExports exports;
    const bool executed = ExecuteModuleFile(module_path, &exports, out_error);
    importing_modules_.erase(normalized_module_id);

    if (!executed) {
        return false;
    }

    imported_modules_.insert(normalized_module_id);
    module_exports_cache_[normalized_module_id] = std::move(exports);
    return true;
}

bool Interpreter::ExecuteModuleFile(const std::filesystem::path& module_path,
                                    ModuleExports* out_exports,
                                    std::string* out_error) {
    std::set<std::string> environment_before;
    std::set<std::string> functions_before;
    std::set<std::string> classes_before;
    if (out_exports != nullptr) {
        out_exports->variables.clear();
        out_exports->functions.clear();
        out_exports->classes.clear();
        for (const auto& entry : environment_) {
            environment_before.insert(entry.first);
        }
        for (const auto& entry : functions_) {
            functions_before.insert(entry.first);
        }
        for (const auto& entry : classes_) {
            classes_before.insert(entry.first);
        }
    }

    std::vector<std::string> lines;
    std::string load_error;
    if (!frontend::LoadSourceLines(module_path.string(), &lines, &load_error)) {
        *out_error = "Error importando modulo '" + module_path.string() + "': " + load_error;
        return false;
    }

    frontend::Parser parser(std::move(lines));
    auto program = std::make_unique<frontend::Program>();
    frontend::Diagnostic diagnostic;
    if (!parser.Parse(program.get(), &diagnostic)) {
        *out_error =
            "Error de parseo importando modulo '" + module_path.string() + "' en linea " +
            std::to_string(diagnostic.line) +
            ", columna " + std::to_string(diagnostic.column) + ": " + diagnostic.message;
        return false;
    }

    module_base_dirs_.push_back(module_path.parent_path());
    const bool executed = ExecuteBlock(program->statements, out_error);
    module_base_dirs_.pop_back();
    if (!executed) {
        return false;
    }

    loaded_module_programs_.push_back(std::move(program));

    if (out_exports != nullptr) {
        for (const auto& entry : environment_) {
            if (environment_before.count(entry.first) == 0) {
                out_exports->variables[entry.first] = entry.second;
            }
        }
        for (const auto& entry : functions_) {
            if (functions_before.count(entry.first) == 0) {
                out_exports->functions.insert(entry.first);
            }
        }
        for (const auto& entry : classes_) {
            if (classes_before.count(entry.first) == 0) {
                out_exports->classes.insert(entry.first);
            }
        }
    }
    return true;
}

std::filesystem::path Interpreter::ResolveModulePath(const std::string& module_name) const {
    const std::filesystem::path current_dir = CurrentModuleBaseDir();
    std::vector<std::filesystem::path> search_roots = CollectAncestorRoots(current_dir);
    // Fall back to the installed standard library (relative to the binary or
    // CLOT_HOME) so imports resolve outside a project checkout. Appended last so
    // local project files always take precedence.
    for (const auto& install_root : runtime::StdlibSearchRoots()) {
        AddUniqueCandidate(&search_roots, install_root);
    }
    std::vector<std::filesystem::path> candidates;
    const auto relative_candidates = DotPathToRelativeCandidates(module_name);

    // 1) Direct relative imports (flat + folder module styles).
    for (const auto& relative_candidate : relative_candidates) {
        AddUniqueCandidate(&candidates, current_dir / relative_candidate);
    }

    // 2) Root-based lookups: scan current directory and every ancestor.
    for (const auto& root : search_roots) {
        AddCandidatesWithRoot(&candidates, root, module_name);
    }

    // 3) Namespace compatibility remaps + canonical package roots across ancestors.
    for (const auto& root : search_roots) {
        AddCandidatesWithPrefixedRoot(
            &candidates, root / "clot" / "science", module_name, "modules.");
        AddCandidatesWithPrefixedRoot(
            &candidates, root / "clot" / "core", module_name, "mods.");

        AddCandidatesWithRoot(&candidates, root / "clot", module_name);
        AddCandidatesWithPrefixedRoot(&candidates, root / "clot", module_name, "clot.");
        AddCandidatesWithRoot(&candidates, root / "clot" / "science", module_name);
        AddCandidatesWithPrefixedRoot(
            &candidates, root / "clot" / "science", module_name, "science.");
        AddCandidatesWithRoot(&candidates, root / "clot" / "core", module_name);
        AddCandidatesWithPrefixedRoot(
            &candidates, root / "clot" / "core", module_name, "core.");
        AddCandidatesWithRoot(&candidates, root / "clot" / "io", module_name);
        AddCandidatesWithPrefixedRoot(
            &candidates, root / "clot" / "io", module_name, "io.");
        AddCandidatesWithRoot(&candidates, root / "clot" / "ml", module_name);
        AddCandidatesWithPrefixedRoot(
            &candidates, root / "clot" / "ml", module_name, "ml.");

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
               ? current_dir / WithClotExtension(DotPathToFolderPath(module_name))
               : candidates.front();
}

runtime::Value Interpreter::BuildModuleAliasValue(const ModuleExports& exports) const {
    runtime::Value::Object object_entries;
    object_entries.reserve(exports.variables.size() + exports.functions.size() + exports.classes.size());

    for (const auto& entry : exports.variables) {
        object_entries.push_back({entry.first, entry.second.value});
    }
    for (const auto& function_name : exports.functions) {
        object_entries.push_back({
            function_name,
            runtime::Value(runtime::Value::FunctionRef{function_name}),
        });
    }
    for (const auto& class_name : exports.classes) {
        object_entries.push_back({class_name, runtime::Value(class_name)});
    }

    return runtime::Value(std::move(object_entries));
}

bool Interpreter::BindImportedSymbol(const frontend::ImportStmt& import_statement,
                                     const ModuleExports& exports,
                                     std::string* out_error) {
    // A whole-module import (with or without alias) exposes the module as a
    // dot-access namespace object. With `as alias` the handle is the alias; a
    // plain `import a.b.c;` uses the last dotted segment (`c`). In both cases the
    // module's top-level symbols are also merged into the current scope (that
    // happens while the module executes), so unqualified access keeps working.
    if (import_statement.style == frontend::ImportStmt::Style::Module ||
        import_statement.style == frontend::ImportStmt::Style::ModuleAlias) {
        std::string namespace_name;
        if (import_statement.style == frontend::ImportStmt::Style::ModuleAlias) {
            if (import_statement.alias_name.empty()) {
                *out_error = "Alias de modulo invalido en import.";
                return false;
            }
            namespace_name = import_statement.alias_name;
        } else {
            namespace_name = LastDottedSegment(import_statement.module_name);
            if (namespace_name.empty()) {
                // Nothing sensible to bind; symbols remain available unqualified.
                return true;
            }
        }

        environment_[namespace_name] = runtime::VariableSlot{
            BuildModuleAliasValue(exports),
            runtime::VariableKind::Dynamic,
        };

        std::vector<std::string> stale_aliases;
        const std::string class_alias_prefix = namespace_name + ".";
        for (const auto& class_alias : class_aliases_) {
            if (StartsWithPrefix(class_alias.first, class_alias_prefix)) {
                stale_aliases.push_back(class_alias.first);
            }
        }
        for (const auto& stale : stale_aliases) {
            class_aliases_.erase(stale);
        }

        for (const auto& class_name : exports.classes) {
            class_aliases_[class_alias_prefix + class_name] = class_name;
        }
        return true;
    }

    const std::string symbol_name = import_statement.imported_symbol;
    const std::string bind_name = import_statement.imported_alias.empty()
                                      ? symbol_name
                                      : import_statement.imported_alias;
    if (symbol_name.empty() || bind_name.empty()) {
        *out_error = "Import directo invalido: simbolo vacio.";
        return false;
    }

    const auto variable = exports.variables.find(symbol_name);
    if (variable != exports.variables.end()) {
        environment_[bind_name] = variable->second;
        class_aliases_.erase(bind_name);
        return true;
    }

    if (exports.functions.count(symbol_name) > 0) {
        environment_[bind_name] = runtime::VariableSlot{
            runtime::Value(runtime::Value::FunctionRef{symbol_name}),
            runtime::VariableKind::Function,
        };
        class_aliases_.erase(bind_name);
        return true;
    }

    if (exports.classes.count(symbol_name) > 0) {
        class_aliases_[bind_name] = symbol_name;
        return true;
    }

    *out_error = "Simbolo '" + symbol_name + "' no exportado por el modulo '" + import_statement.module_name + "'.";
    return false;
}

std::filesystem::path Interpreter::CurrentModuleBaseDir() const {
    if (!module_base_dirs_.empty()) {
        return module_base_dirs_.back();
    }
    if (!entry_file_path_.empty()) {
        return entry_file_path_.parent_path();
    }
    return std::filesystem::current_path();
}

}  // namespace clot::interpreter
