#ifndef CLOT_INTERPRETER_INTERPRETER_HPP
#define CLOT_INTERPRETER_INTERPRETER_HPP

#include <filesystem>
#include <future>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "clot/frontend/ast.hpp"
#include "clot/runtime/value.hpp"

namespace clot::interpreter {

class Interpreter {
public:
    void SetEntryFilePath(const std::string& file_path);

    bool Execute(const frontend::Program& program, std::string* out_error);

private:
    bool ExecuteStatement(const frontend::Statement& statement, std::string* out_error);
    bool ExecuteBlock(const std::vector<std::unique_ptr<frontend::Statement>>& statements, std::string* out_error);
    bool ExecuteMutation(const frontend::MutationStmt& statement, std::string* out_error);
    bool ExecuteReturn(const frontend::ReturnStmt& statement, std::string* out_error);
    bool ExecuteTryCatch(const frontend::TryCatchStmt& statement, std::string* out_error);

    bool EvaluateExpression(
        const frontend::Expr& expression,
        runtime::Value* out_value,
        std::string* out_error);

    bool EvaluateUnary(
        frontend::UnaryOp op,
        const runtime::Value& operand,
        runtime::Value* out_value,
        std::string* out_error) const;

    bool EvaluateBinary(
        frontend::BinaryOp op,
        const runtime::Value& lhs,
        const runtime::Value& rhs,
        runtime::Value* out_value,
        std::string* out_error) const;

    bool ExecuteCall(
        const frontend::CallExpr& call,
        bool require_return_value,
        runtime::Value* out_value,
        std::string* out_error);

    bool ExecuteBuiltinCall(
        const frontend::CallExpr& call,
        bool* out_was_builtin,
        runtime::Value* out_value,
        std::string* out_error);

    bool ExecuteUserFunction(
        const frontend::FunctionDeclStmt& function,
        const frontend::CallExpr& call,
        bool require_return_value,
        runtime::Value* out_value,
        std::string* out_error);

    bool ResolveVariable(
        const std::string& name,
        runtime::Value* out_value,
        std::string* out_error) const;

    bool ResolveMutableVariable(
        const std::string& name,
        bool create_missing_property,
        runtime::Value** out_value,
        std::string* out_error);

    bool ResolveMutableTarget(
        const frontend::Expr& target,
        bool create_missing_property,
        runtime::Value** out_value,
        std::string* out_error);

    bool NormalizeValueForKind(
        runtime::VariableKind kind,
        const runtime::Value& value,
        runtime::Value* out_value,
        std::string* out_error) const;

    bool AssignValue(
        const frontend::AssignmentStmt& statement,
        const runtime::Value& value,
        std::string* out_error);

    bool ApplyVariableMutation(
        const std::string& name,
        frontend::AssignmentOp op,
        const runtime::Value& value,
        std::string* out_error);

    bool ApplyTargetMutation(
        const frontend::Expr& target,
        frontend::AssignmentOp op,
        const runtime::Value& value,
        std::string* out_error);

    bool ImportModule(const std::string& module_name, std::string* out_error);
    bool ExecuteModuleFile(const std::filesystem::path& module_path, std::string* out_error);
    std::filesystem::path ResolveModulePath(const std::string& module_name) const;
    std::filesystem::path CurrentModuleBaseDir() const;

    std::map<std::string, runtime::VariableSlot> environment_;
    std::map<std::string, const frontend::FunctionDeclStmt*> functions_;
    std::set<std::string> imported_modules_;
    std::set<std::string> importing_modules_;
    std::vector<std::optional<runtime::Value>> return_stack_;
    std::vector<std::filesystem::path> module_base_dirs_;
    std::filesystem::path entry_file_path_;
    std::vector<std::unique_ptr<frontend::Program>> loaded_module_programs_;

    struct AsyncTaskResult {
        bool ok = false;
        runtime::Value value;
        std::string error;
    };

    struct AsyncTaskState {
        std::future<AsyncTaskResult> future;
    };

    std::unordered_map<long long, AsyncTaskState> async_tasks_;
    long long next_async_task_id_ = 1;
};

}  // namespace clot::interpreter

#endif  // CLOT_INTERPRETER_INTERPRETER_HPP
