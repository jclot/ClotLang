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
    bool RaiseExceptionValue(const runtime::Value& value, std::string* out_error);

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

    bool ExecuteCallable(
        const std::string& callable_name,
        frontend::TypeHint return_type,
        const std::vector<frontend::FunctionParam>& params,
        const std::vector<std::unique_ptr<frontend::Statement>>& body,
        const frontend::CallExpr& call,
        bool require_return_value,
        runtime::Value* out_value,
        std::string* out_error,
        runtime::Value* bound_this = nullptr);

    bool ExecuteClassCallable(
        const std::string& class_name,
        const std::string& callable_name,
        frontend::TypeHint return_type,
        const std::vector<frontend::FunctionParam>& params,
        const std::vector<std::unique_ptr<frontend::Statement>>& body,
        const frontend::CallExpr& call,
        bool require_return_value,
        runtime::Value* out_value,
        std::string* out_error,
        runtime::Value* bound_this = nullptr,
        bool is_constructor = false,
        bool* out_constructor_called_super = nullptr);

    bool ExecuteUserFunction(
        const frontend::FunctionDeclStmt& function,
        const frontend::CallExpr& call,
        bool require_return_value,
        runtime::Value* out_value,
        std::string* out_error);

    bool ExecuteInterfaceDeclaration(const frontend::InterfaceDeclStmt& declaration, std::string* out_error);
    bool ExecuteClassDeclaration(const frontend::ClassDeclStmt& declaration, std::string* out_error);
    bool ExecuteSuperCall(
        const frontend::CallExpr& call,
        bool require_return_value,
        runtime::Value* out_value,
        std::string* out_error);
    bool InstantiateClass(
        const frontend::ClassDeclStmt& declaration,
        const frontend::CallExpr& call,
        runtime::Value* out_value,
        std::string* out_error);
    bool InitializeInstanceFields(
        const frontend::ClassDeclStmt& declaration,
        runtime::Value* instance,
        std::string* out_error);
    bool ExecuteClassConstructor(
        const frontend::ClassDeclStmt& declaration,
        runtime::Value* instance,
        const frontend::CallExpr& call,
        std::string* out_error);
    bool ExecuteClassSetter(
        runtime::Value* instance,
        const std::string& class_name,
        const std::string& property_name,
        const runtime::Value& value,
        std::string* out_error);
    bool TryExecuteClassGetter(
        runtime::Value* instance,
        const std::string& class_name,
        const std::string& property_name,
        runtime::Value* out_value,
        std::string* out_error);

    const frontend::ClassDeclStmt* FindClass(const std::string& class_name) const;
    bool IsClassInstance(const runtime::Value& value, std::string* out_class_name) const;
    bool ResolveClassField(
        const frontend::ClassDeclStmt& declaration,
        const std::string& field_name,
        const frontend::ClassFieldDecl** out_field,
        std::string* out_owner_class) const;
    bool ResolveClassMethod(
        const frontend::ClassDeclStmt& declaration,
        const std::string& method_name,
        const frontend::ClassMethodDecl** out_method,
        std::string* out_owner_class) const;
    bool ResolveClassAccessor(
        const frontend::ClassDeclStmt& declaration,
        const std::string& property_name,
        bool setter,
        const frontend::ClassAccessorDecl** out_accessor,
        std::string* out_owner_class) const;
    bool HasClassContextAccess(const std::string& owner_class) const;
    bool CanAccessMember(frontend::MemberVisibility visibility, const std::string& owner_class) const;
    bool ValidateInterfaceImplementation(const frontend::ClassDeclStmt& declaration, std::string* out_error) const;
    bool ValidateOverrideRules(const frontend::ClassDeclStmt& declaration, std::string* out_error) const;
    bool ResolveClassStaticField(
        const std::string& class_name,
        const std::string& field_name,
        runtime::Value** out_value,
        bool create_missing,
        std::string* out_error);

    struct RuntimeExceptionRecord {
        std::string type_name;
        runtime::Value payload;
        std::string message;
    };

    RuntimeExceptionRecord BuildRuntimeExceptionFromError(const std::string& runtime_error) const;
    std::string InferRuntimeExceptionTypeName(const std::string& runtime_error) const;
    bool ExceptionMatchesCatchType(const RuntimeExceptionRecord& exception, const std::string& catch_type) const;
    bool IsClassTypeOrDerived(const std::string& type_name, const std::string& base_type_name) const;

    bool ResolveVariable(
        const std::string& name,
        runtime::Value* out_value,
        std::string* out_error);

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
    bool NormalizeValueForTypeHint(
        frontend::TypeHint hint,
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

    struct ModuleExports {
        std::map<std::string, runtime::VariableSlot> variables;
        std::set<std::string> functions;
        std::set<std::string> classes;
    };

    bool ImportModule(const std::string& module_name, std::string* out_module_id, std::string* out_error);
    bool ExecuteModuleFile(const std::filesystem::path& module_path, ModuleExports* out_exports, std::string* out_error);
    bool BindImportedSymbol(const frontend::ImportStmt& import_statement,
                            const ModuleExports& exports,
                            std::string* out_error);
    runtime::Value BuildModuleAliasValue(const ModuleExports& exports) const;
    std::filesystem::path ResolveModulePath(const std::string& module_name) const;
    std::filesystem::path CurrentModuleBaseDir() const;

    std::map<std::string, runtime::VariableSlot> environment_;
    std::map<std::string, const frontend::FunctionDeclStmt*> functions_;
    std::unordered_map<std::string, const frontend::InterfaceDeclStmt*> interfaces_;
    struct ClassRuntimeInfo {
        const frontend::ClassDeclStmt* declaration = nullptr;
        std::map<std::string, runtime::VariableSlot> static_fields;
        std::set<std::string> readonly_static_fields;
    };
    std::unordered_map<std::string, ClassRuntimeInfo> classes_;
    std::set<std::string> imported_modules_;
    std::set<std::string> importing_modules_;
    std::vector<std::optional<runtime::Value>> return_stack_;
    std::vector<std::string> class_execution_stack_;
    std::vector<std::string> constructor_execution_stack_;
    std::vector<bool> constructor_super_called_stack_;
    std::vector<runtime::Value*> constructor_instance_stack_;
    std::vector<std::filesystem::path> module_base_dirs_;
    std::filesystem::path entry_file_path_;
    std::vector<std::unique_ptr<frontend::Program>> loaded_module_programs_;
    std::unordered_map<std::string, ModuleExports> module_exports_cache_;
    std::unordered_map<std::string, std::string> class_aliases_;
    std::optional<RuntimeExceptionRecord> pending_exception_;

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
