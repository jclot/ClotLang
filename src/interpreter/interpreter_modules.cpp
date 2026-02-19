#include "clot/interpreter/interpreter.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "clot/frontend/parser.hpp"
#include "clot/frontend/source_loader.hpp"

namespace clot::interpreter {

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
    std::string relative_module = module_name;
    std::replace(relative_module.begin(), relative_module.end(), '.', std::filesystem::path::preferred_separator);

    std::filesystem::path candidate = CurrentModuleBaseDir() / relative_module;
    if (candidate.extension().empty()) {
        candidate += ".clot";
    }
    return candidate;
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
