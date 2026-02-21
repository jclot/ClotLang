#include "llvm_backend_internal.hpp"

#ifdef CLOT_HAS_LLVM

#include "clot/runtime/i18n.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

#include <llvm/Support/raw_ostream.h>

namespace clot::codegen::internal {

namespace {

std::string QuoteForShell(const std::string& value) {
    std::string quoted = "'";
    for (char character : value) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(character);
        }
    }
    quoted += "'";
    return quoted;
}

bool RuntimeBridgeSourcesExist(
    const std::filesystem::path& bridge_source,
    const std::filesystem::path& external_bridge_source,
    const std::filesystem::path& parser_core_source,
    const std::filesystem::path& parser_expression_source,
    const std::filesystem::path& parser_statements_source,
    const std::filesystem::path& source_loader_source,
    const std::filesystem::path& tokenizer_source,
    const std::filesystem::path& interpreter_source,
    const std::filesystem::path& interpreter_state_source,
    const std::filesystem::path& interpreter_modules_source,
    const std::filesystem::path& i18n_source) {
    return std::filesystem::exists(bridge_source) &&
           std::filesystem::exists(external_bridge_source) &&
           std::filesystem::exists(parser_core_source) &&
           std::filesystem::exists(parser_expression_source) &&
           std::filesystem::exists(parser_statements_source) &&
           std::filesystem::exists(source_loader_source) &&
           std::filesystem::exists(tokenizer_source) &&
           std::filesystem::exists(interpreter_source) &&
           std::filesystem::exists(interpreter_state_source) &&
           std::filesystem::exists(interpreter_modules_source) &&
           std::filesystem::exists(i18n_source);
}

}  // namespace

bool LinkExecutable(
    const std::string& object_path,
    const std::string& executable_path,
    bool use_runtime_bridge,
    const CompileOptions& options,
    bool verbose,
    std::string* out_error) {
    std::string command = "clang++ ";

    if (use_runtime_bridge) {
        if (options.project_root.empty()) {
            *out_error = "project_root vacio: no se puede enlazar runtime bridge LLVM.";
            return false;
        }

        const std::filesystem::path root = std::filesystem::path(options.project_root);
        const std::filesystem::path include_dir = root / "include";
        const std::filesystem::path bridge_source = root / "src" / "codegen" / "runtime_bridge.cpp";
        const std::filesystem::path external_bridge_source = root / "src" / "codegen" / "runtime_bridge_external.cpp";

        const std::filesystem::path parser_core_source = root / "src" / "frontend" / "parser_core.cpp";
        const std::filesystem::path parser_expression_source = root / "src" / "frontend" / "parser_expression.cpp";
        const std::filesystem::path parser_statements_source = root / "src" / "frontend" / "parser_statements.cpp";

        const std::filesystem::path source_loader_source = root / "src" / "frontend" / "source_loader.cpp";
        const std::filesystem::path tokenizer_source = root / "src" / "frontend" / "tokenizer.cpp";
        const std::filesystem::path interpreter_source = root / "src" / "interpreter" / "interpreter.cpp";
        const std::filesystem::path interpreter_state_source = root / "src" / "interpreter" / "interpreter_state.cpp";
        const std::filesystem::path interpreter_modules_source = root / "src" / "interpreter" / "interpreter_modules.cpp";
        const std::filesystem::path i18n_source = root / "src" / "runtime" / "i18n.cpp";

        const bool use_external_bridge = options.runtime_bridge_mode == CompileOptions::RuntimeBridgeMode::External;
        if (use_external_bridge) {
            if (!std::filesystem::exists(external_bridge_source)) {
                *out_error = "No se encontro runtime bridge externo LLVM en: " + external_bridge_source.string();
                return false;
            }
        } else if (!RuntimeBridgeSourcesExist(
                       bridge_source,
                       external_bridge_source,
                       parser_core_source,
                       parser_expression_source,
                       parser_statements_source,
                       source_loader_source,
                       tokenizer_source,
                       interpreter_source,
                       interpreter_state_source,
                       interpreter_modules_source,
                       i18n_source)) {
            *out_error = "No se encontraron archivos fuente para runtime bridge LLVM en: " + root.string();
            return false;
        }

        command += "-std=c++20 -O2 ";
        command += "-I" + QuoteForShell(include_dir.string()) + " ";
#ifndef _WIN32
        command += "-no-pie ";
#endif
        command += QuoteForShell(object_path) + " ";
        if (use_external_bridge) {
            command += "-DCLOT_EXTERNAL_RUNTIME_BRIDGE_IMPL ";
            command += QuoteForShell(external_bridge_source.string()) + " ";
        } else {
            command += QuoteForShell(bridge_source.string()) + " ";
            command += QuoteForShell(parser_core_source.string()) + " ";
            command += QuoteForShell(parser_expression_source.string()) + " ";
            command += QuoteForShell(parser_statements_source.string()) + " ";
            command += QuoteForShell(source_loader_source.string()) + " ";
            command += QuoteForShell(tokenizer_source.string()) + " ";
            command += QuoteForShell(interpreter_source.string()) + " ";
            command += QuoteForShell(interpreter_state_source.string()) + " ";
            command += QuoteForShell(interpreter_modules_source.string()) + " ";
            command += QuoteForShell(i18n_source.string()) + " ";
        }
        command += "-o " + QuoteForShell(executable_path);
    } else {
#ifndef _WIN32
        command += "-no-pie ";
#endif
        command += QuoteForShell(object_path) + " -o " + QuoteForShell(executable_path);
    }

    if (verbose) {
        if (use_runtime_bridge) {
            if (options.runtime_bridge_mode == CompileOptions::RuntimeBridgeMode::External) {
                llvm::outs() << clot::runtime::Tr(
                    "[clot] runtime bridge externo activado (binario liviano, requiere clot en PATH)\n",
                    "[clot] external runtime bridge enabled (light binary, requires clot in PATH)\n");
            } else {
                llvm::outs() << clot::runtime::Tr(
                    "[clot] runtime bridge LLVM activado para soporte completo del lenguaje\n",
                    "[clot] LLVM runtime bridge enabled for full language support\n");
            }
        }
        llvm::outs() << "[clot] linking: " << command << "\n";
    }

    const int status = std::system(command.c_str());
    if (status != 0) {
        *out_error = "Fallo el enlazado con clang++. Comando: " + command;
        return false;
    }

    return true;
}

}  // namespace clot::codegen::internal

#endif  // CLOT_HAS_LLVM
