#include "clot/interpreter/interpreter.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "clot/frontend/parser.hpp"
#include "clot/frontend/source_loader.hpp"

namespace clot::interpreter {

namespace {

bool StartsWithPrefix(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
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
    std::replace(module_name.begin(), module_name.end(), '.', std::filesystem::path::preferred_separator);
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

}  // namespace

bool Interpreter::ImportModule(const std::string& module_name, std::string* out_error) {
    if (module_name == "math") {
        imported_modules_.insert(module_name);
        return true;
    }

    const std::filesystem::path module_path = ResolveModulePath(module_name);
    std::error_code ec;
    const std::string module_id = std::filesystem::weakly_canonical(module_path, ec).string();
    const std::string normalized_module_id = ec ? module_path.lexically_normal().string() : module_id;

    if (imported_modules_.count(normalized_module_id) > 0) {
        return true;
    }

    if (importing_modules_.count(normalized_module_id) > 0) {
        *out_error = "Import circular detectado en modulo: " + normalized_module_id;
        return false;
    }

    importing_modules_.insert(normalized_module_id);
    const bool executed = ExecuteModuleFile(module_path, out_error);
    importing_modules_.erase(normalized_module_id);

    if (!executed) {
        return false;
    }

    imported_modules_.insert(normalized_module_id);
    return true;
}

bool Interpreter::ExecuteModuleFile(const std::filesystem::path& module_path, std::string* out_error) {
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
    if (executed) {
        loaded_module_programs_.push_back(std::move(program));
    }
    return executed;
}

std::filesystem::path Interpreter::ResolveModulePath(const std::string& module_name) const {
    const std::filesystem::path current_dir = CurrentModuleBaseDir();
    const std::filesystem::path project_dir =
        entry_file_path_.empty() ? current_dir : entry_file_path_.parent_path();
    std::vector<std::filesystem::path> candidates;
    const auto relative_candidates = DotPathToRelativeCandidates(module_name);

    // 1) Direct relative imports (flat + folder module styles).
    for (const auto& relative_candidate : relative_candidates) {
        AddUniqueCandidate(&candidates, current_dir / relative_candidate);
    }

    // 2) Namespace compatibility remaps.
    AddCandidatesWithPrefixedRoot(
        &candidates, project_dir / "clot" / "science", module_name, "modules.");
    AddCandidatesWithPrefixedRoot(
        &candidates, project_dir / "clot" / "core", module_name, "mods.");

    // 3) Canonical package root: clot/{science,core,io,ml}.
    AddCandidatesWithRoot(&candidates, project_dir / "clot", module_name);
    AddCandidatesWithPrefixedRoot(&candidates, project_dir / "clot", module_name, "clot.");
    AddCandidatesWithRoot(&candidates, project_dir / "clot" / "science", module_name);
    AddCandidatesWithPrefixedRoot(
        &candidates, project_dir / "clot" / "science", module_name, "science.");
    AddCandidatesWithRoot(&candidates, project_dir / "clot" / "core", module_name);
    AddCandidatesWithPrefixedRoot(
        &candidates, project_dir / "clot" / "core", module_name, "core.");
    AddCandidatesWithRoot(&candidates, project_dir / "clot" / "io", module_name);
    AddCandidatesWithPrefixedRoot(
        &candidates, project_dir / "clot" / "io", module_name, "io.");
    AddCandidatesWithRoot(&candidates, project_dir / "clot" / "ml", module_name);
    AddCandidatesWithPrefixedRoot(
        &candidates, project_dir / "clot" / "ml", module_name, "ml.");

    // 4) Backward compatibility roots.
    AddCandidatesWithRoot(&candidates, project_dir / "modules", module_name);
    AddCandidatesWithRoot(&candidates, project_dir / "mods", module_name);
    AddCandidatesWithPrefixedRoot(&candidates, project_dir / "modules", module_name, "modules.");
    AddCandidatesWithPrefixedRoot(&candidates, project_dir / "mods", module_name, "mods.");

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return candidates.empty()
               ? current_dir / WithClotExtension(std::filesystem::path(module_name))
               : candidates.front();
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
