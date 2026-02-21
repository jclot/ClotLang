#ifndef CLOT_CODEGEN_LLVM_COMPILER_HPP
#define CLOT_CODEGEN_LLVM_COMPILER_HPP

#include <string>

#include "clot/frontend/ast.hpp"

namespace clot::codegen {

struct CompileOptions {
    enum class EmitKind {
        Executable,
        Object,
        IR,
    };

    enum class RuntimeBridgeMode {
        Static,
        External,
    };

    EmitKind emit_kind = EmitKind::Executable;
    std::string output_path;
    std::string target_triple;
    std::string input_path;
    std::string source_text;
    std::string project_root;
    RuntimeBridgeMode runtime_bridge_mode = RuntimeBridgeMode::Static;
    bool verbose = false;
};

class LlvmCompiler {
public:
    static bool IsAvailable();

    bool Compile(
        const frontend::Program& program,
        const CompileOptions& options,
        std::string* out_error) const;
};

}  // namespace clot::codegen

#endif  // CLOT_CODEGEN_LLVM_COMPILER_HPP
