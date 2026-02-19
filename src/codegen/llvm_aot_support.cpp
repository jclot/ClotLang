#include "llvm_backend_internal.hpp"

#ifdef CLOT_HAS_LLVM

#include <algorithm>
#include <utility>

namespace clot::codegen::internal {

bool ContainsDot(const std::string& value) {
    return value.find('.') != std::string::npos;
}

bool ContainsMathImportInStatement(const frontend::Statement& statement) {
    if (const auto* import_stmt = dynamic_cast<const frontend::ImportStmt*>(&statement)) {
        return import_stmt->module_name == "math";
    }

    if (const auto* conditional = dynamic_cast<const frontend::IfStmt*>(&statement)) {
        for (const auto& nested : conditional->then_branch) {
            if (nested != nullptr && ContainsMathImportInStatement(*nested)) {
                return true;
            }
        }

        for (const auto& nested : conditional->else_branch) {
            if (nested != nullptr && ContainsMathImportInStatement(*nested)) {
                return true;
            }
        }
    }

    if (const auto* function_decl = dynamic_cast<const frontend::FunctionDeclStmt*>(&statement)) {
        for (const auto& nested : function_decl->body) {
            if (nested != nullptr && ContainsMathImportInStatement(*nested)) {
                return true;
            }
        }
    }

    return false;
}

bool CollectAotSupportContext(const frontend::Program& program, AotSupportContext* out_context) {
    AotSupportContext context;

    for (const auto& statement : program.statements) {
        if (statement == nullptr) {
            return false;
        }

        if (const auto* function_decl = dynamic_cast<const frontend::FunctionDeclStmt*>(statement.get())) {
            if (context.functions.find(function_decl->name) != context.functions.end()) {
                return false;
            }

            FunctionSignature signature;
            signature.by_reference_params.reserve(function_decl->params.size());
            for (const auto& param : function_decl->params) {
                signature.by_reference_params.push_back(param.by_reference);
            }
            context.functions[function_decl->name] = std::move(signature);
        }

        if (ContainsMathImportInStatement(*statement)) {
            context.math_module_imported = true;
        }
    }

    *out_context = std::move(context);
    return true;
}

bool IsAotSupportedExpr(const frontend::Expr& expression, const AotSupportContext& context) {
    if (dynamic_cast<const frontend::NumberExpr*>(&expression) != nullptr) {
        return true;
    }

    if (dynamic_cast<const frontend::BoolExpr*>(&expression) != nullptr) {
        return true;
    }

    if (const auto* string_expr = dynamic_cast<const frontend::StringExpr*>(&expression)) {
        (void)string_expr;
        return false;
    }

    if (const auto* variable = dynamic_cast<const frontend::VariableExpr*>(&expression)) {
        return !ContainsDot(variable->name);
    }

    if (dynamic_cast<const frontend::ListExpr*>(&expression) != nullptr) {
        return false;
    }

    if (dynamic_cast<const frontend::ObjectExpr*>(&expression) != nullptr) {
        return false;
    }

    if (dynamic_cast<const frontend::IndexExpr*>(&expression) != nullptr) {
        return false;
    }

    if (const auto* call = dynamic_cast<const frontend::CallExpr*>(&expression)) {
        if (call->callee != "sum" || call->arguments.size() != 2 || !context.math_module_imported) {
            return false;
        }

        for (const auto& argument : call->arguments) {
            if (argument.by_reference || argument.value == nullptr || !IsAotSupportedExpr(*argument.value, context)) {
                return false;
            }
        }
        return true;
    }

    if (const auto* unary = dynamic_cast<const frontend::UnaryExpr*>(&expression)) {
        return unary->operand != nullptr && IsAotSupportedExpr(*unary->operand, context);
    }

    if (const auto* binary = dynamic_cast<const frontend::BinaryExpr*>(&expression)) {
        return binary->lhs != nullptr && binary->rhs != nullptr &&
               IsAotSupportedExpr(*binary->lhs, context) &&
               IsAotSupportedExpr(*binary->rhs, context);
    }

    return false;
}

bool IsAotSupportedCallStatement(const frontend::CallExpr& call, const AotSupportContext& context) {
    if (call.callee == "sum" && context.math_module_imported) {
        return IsAotSupportedExpr(call, context);
    }

    const auto function_it = context.functions.find(call.callee);
    if (function_it == context.functions.end()) {
        return false;
    }

    const std::vector<bool>& by_reference_params = function_it->second.by_reference_params;
    if (call.arguments.size() != by_reference_params.size()) {
        return false;
    }

    for (std::size_t i = 0; i < call.arguments.size(); ++i) {
        const frontend::CallArgument& argument = call.arguments[i];
        if (argument.value == nullptr) {
            return false;
        }

        if (by_reference_params[i]) {
            const auto* variable = dynamic_cast<const frontend::VariableExpr*>(argument.value.get());
            if (variable == nullptr || ContainsDot(variable->name)) {
                return false;
            }
            continue;
        }

        if (argument.by_reference || !IsAotSupportedExpr(*argument.value, context)) {
            return false;
        }
    }

    return true;
}

bool IsAotSupportedStatement(
    const frontend::Statement& statement,
    const AotSupportContext& context,
    bool inside_function) {
    if (const auto* assignment = dynamic_cast<const frontend::AssignmentStmt*>(&statement)) {
        return !ContainsDot(assignment->name) &&
               assignment->expr != nullptr &&
               IsAotSupportedExpr(*assignment->expr, context);
    }

    if (const auto* print = dynamic_cast<const frontend::PrintStmt*>(&statement)) {
        if (print->expr == nullptr) {
            return false;
        }
        if (dynamic_cast<const frontend::StringExpr*>(print->expr.get()) != nullptr) {
            return true;
        }
        return IsAotSupportedExpr(*print->expr, context);
    }

    if (const auto* conditional = dynamic_cast<const frontend::IfStmt*>(&statement)) {
        if (conditional->condition == nullptr || !IsAotSupportedExpr(*conditional->condition, context)) {
            return false;
        }

        const bool then_supported = std::all_of(
            conditional->then_branch.begin(),
            conditional->then_branch.end(),
            [&context](const std::unique_ptr<frontend::Statement>& nested) {
                return nested != nullptr && IsAotSupportedStatement(*nested, context, true);
            });

        const bool else_supported = std::all_of(
            conditional->else_branch.begin(),
            conditional->else_branch.end(),
            [&context](const std::unique_ptr<frontend::Statement>& nested) {
                return nested != nullptr && IsAotSupportedStatement(*nested, context, true);
            });

        return then_supported && else_supported;
    }

    if (const auto* import_stmt = dynamic_cast<const frontend::ImportStmt*>(&statement)) {
        return import_stmt->module_name == "math";
    }

    if (const auto* expression_stmt = dynamic_cast<const frontend::ExpressionStmt*>(&statement)) {
        if (expression_stmt->expr == nullptr) {
            return false;
        }

        if (const auto* call = dynamic_cast<const frontend::CallExpr*>(expression_stmt->expr.get())) {
            return IsAotSupportedCallStatement(*call, context);
        }

        return IsAotSupportedExpr(*expression_stmt->expr, context);
    }

    if (const auto* function_decl = dynamic_cast<const frontend::FunctionDeclStmt*>(&statement)) {
        if (inside_function) {
            return false;
        }

        return std::all_of(
            function_decl->body.begin(),
            function_decl->body.end(),
            [&context](const std::unique_ptr<frontend::Statement>& nested) {
                return nested != nullptr && IsAotSupportedStatement(*nested, context, true);
            });
    }

    return false;
}

bool IsAotSupportedProgram(const frontend::Program& program) {
    AotSupportContext context;
    if (!CollectAotSupportContext(program, &context)) {
        return false;
    }

    return std::all_of(
        program.statements.begin(),
        program.statements.end(),
        [&context](const std::unique_ptr<frontend::Statement>& statement) {
            return statement != nullptr && IsAotSupportedStatement(*statement, context, false);
        });
}

}  // namespace clot::codegen::internal

#endif  // CLOT_HAS_LLVM
