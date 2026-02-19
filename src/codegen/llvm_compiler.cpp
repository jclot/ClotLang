#include "clot/codegen/llvm_compiler.hpp"

#ifdef CLOT_HAS_LLVM

#include "llvm_backend_internal.hpp"

#include <filesystem>

#endif

namespace clot::codegen {

bool LlvmCompiler::IsAvailable() {
#ifdef CLOT_HAS_LLVM
    return true;
#else
    return false;
#endif
}

#ifdef CLOT_HAS_LLVM

bool LlvmCompiler::Compile(
    const frontend::Program& program,
    const CompileOptions& options,
    std::string* out_error) const {
    if (options.output_path.empty()) {
        *out_error = "Se requiere output_path para compilar con LLVM.";
        return false;
    }

    internal::LlvmEmitter emitter("clot_module");
    if (!emitter.EmitProgram(program, options, out_error)) {
        return false;
    }

    if (options.emit_kind == CompileOptions::EmitKind::IR) {
        return emitter.EmitIRFile(options.output_path, out_error);
    }

    if (options.emit_kind == CompileOptions::EmitKind::Object) {
        return emitter.EmitObjectFile(options.output_path, options.target_triple, out_error);
    }

    const std::filesystem::path executable_path(options.output_path);
    const std::filesystem::path object_path = executable_path.string() + ".o";

    if (!emitter.EmitObjectFile(object_path.string(), options.target_triple, out_error)) {
        return false;
    }

    if (!internal::LinkExecutable(
            object_path.string(),
            executable_path.string(),
            emitter.UsedRuntimeBridge(),
            options,
            options.verbose,
            out_error)) {
        return false;
    }

    std::error_code ignored;
    std::filesystem::remove(object_path, ignored);
    return true;
}

#else

bool LlvmCompiler::Compile(
    const frontend::Program&,
    const CompileOptions&,
    std::string* out_error) const {
    if (out_error != nullptr) {
        *out_error = "Este binario se compilo sin soporte LLVM. Reconfigura con LLVM instalado.";
    }
    return false;
}

#endif

}  // namespace clot::codegen
