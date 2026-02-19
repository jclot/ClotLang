#ifndef CLOT_CODEGEN_LLVM_BACKEND_INTERNAL_HPP
#define CLOT_CODEGEN_LLVM_BACKEND_INTERNAL_HPP

#ifdef CLOT_HAS_LLVM

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "clot/codegen/llvm_compiler.hpp"

namespace clot::codegen::internal {

struct FunctionSignature {
    std::vector<bool> by_reference_params;
};

struct AotSupportContext {
    std::unordered_map<std::string, FunctionSignature> functions;
    bool math_module_imported = false;
};

bool ContainsDot(const std::string& value);

bool ContainsMathImportInStatement(const frontend::Statement& statement);
bool CollectAotSupportContext(const frontend::Program& program, AotSupportContext* out_context);
bool IsAotSupportedExpr(const frontend::Expr& expression, const AotSupportContext& context);
bool IsAotSupportedCallStatement(const frontend::CallExpr& call, const AotSupportContext& context);
bool IsAotSupportedStatement(
    const frontend::Statement& statement,
    const AotSupportContext& context,
    bool inside_function);
bool IsAotSupportedProgram(const frontend::Program& program);

class LlvmEmitter {
public:
    explicit LlvmEmitter(std::string module_name);

    bool EmitProgram(const frontend::Program& program, const CompileOptions& options, std::string* out_error);
    bool UsedRuntimeBridge() const;

    bool EmitIRFile(const std::string& output_path, std::string* out_error);
    bool EmitObjectFile(
        const std::string& output_path,
        const std::string& requested_target,
        std::string* out_error);

private:
    struct UserFunctionInfo {
        const frontend::FunctionDeclStmt* declaration = nullptr;
        llvm::Function* llvm_function = nullptr;
        std::vector<bool> param_by_reference;
    };

    bool EmitRuntimeBridgeProgram(const CompileOptions& options);
    bool CreateMainFunction();
    bool EnsurePrintfFunction();
    bool DeclareUserFunctions(const frontend::Program& program);
    bool EmitUserFunctions();
    bool EmitUserFunction(const UserFunctionInfo& function_info);
    bool EmitStatement(const frontend::Statement& statement, bool allow_function_declaration);
    bool EmitAssignment(const frontend::AssignmentStmt& statement);
    bool EmitPrint(const frontend::PrintStmt& statement);
    bool EmitIf(const frontend::IfStmt& statement);
    bool EmitCallStatement(const frontend::CallExpr& call);

    llvm::AllocaInst* CreateEntryBlockAlloca(llvm::Function* function, const std::string& name);
    llvm::Value* EmitBuiltinSumCall(const frontend::CallExpr& call);
    bool EmitUserFunctionCall(
        const frontend::CallExpr& call,
        bool require_numeric_result,
        llvm::Value** out_value);
    llvm::Value* EmitNumericExpr(const frontend::Expr& expression);
    llvm::Value* BoolToNumber(llvm::Value* bool_value);

    std::string MangleFunctionName(const std::string& name) const;

    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;
    llvm::IRBuilder<> builder_;

    llvm::Function* main_function_ = nullptr;
    llvm::Function* current_function_ = nullptr;
    llvm::FunctionCallee printf_function_;

    std::unordered_map<std::string, llvm::Value*> variables_;
    std::unordered_map<std::string, UserFunctionInfo> user_functions_;
    std::vector<std::string> user_function_order_;
    bool math_module_imported_ = false;
    bool use_runtime_bridge_ = false;
    std::string error_;
};

bool LinkExecutable(
    const std::string& object_path,
    const std::string& executable_path,
    bool use_runtime_bridge,
    const CompileOptions& options,
    bool verbose,
    std::string* out_error);

}  // namespace clot::codegen::internal

#endif  // CLOT_HAS_LLVM

#endif  // CLOT_CODEGEN_LLVM_BACKEND_INTERNAL_HPP
