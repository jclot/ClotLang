#include "clot/interpreter/interpreter.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>

namespace clot::interpreter {

namespace {

bool ReadNumeric(const runtime::Value& value, double* out_number, std::string* out_error) {
    bool ok = false;
    const double numeric = value.AsNumber(&ok);
    if (!ok) {
        if (out_error != nullptr) {
            *out_error = "La expresion requiere un valor numerico.";
        }
        return false;
    }

    *out_number = numeric;
    return true;
}

bool ReadInteger(const runtime::Value& value, long long* out_integer) {
    bool ok = false;
    const long long integer = value.AsInteger(&ok);
    if (!ok) {
        return false;
    }

    *out_integer = integer;
    return true;
}

bool CheckedAdd(long long lhs, long long rhs, long long* out_result) {
    if (rhs > 0 && lhs > std::numeric_limits<long long>::max() - rhs) {
        return false;
    }
    if (rhs < 0 && lhs < std::numeric_limits<long long>::min() - rhs) {
        return false;
    }

    *out_result = lhs + rhs;
    return true;
}

bool CheckedSubtract(long long lhs, long long rhs, long long* out_result) {
    if (rhs > 0 && lhs < std::numeric_limits<long long>::min() + rhs) {
        return false;
    }
    if (rhs < 0 && lhs > std::numeric_limits<long long>::max() + rhs) {
        return false;
    }

    *out_result = lhs - rhs;
    return true;
}

bool ReadListIndex(const runtime::Value& value, std::size_t* out_index, std::string* out_error) {
    double numeric_index = 0.0;
    if (!ReadNumeric(value, &numeric_index, out_error)) {
        return false;
    }

    if (!std::isfinite(numeric_index) ||
        std::trunc(numeric_index) != numeric_index ||
        numeric_index < 0.0 ||
        numeric_index >= 9223372036854775808.0) {  // 2^63
        if (out_error != nullptr) {
            *out_error = "El indice de lista debe ser un entero finito.";
        }
        return false;
    }

    *out_index = static_cast<std::size_t>(static_cast<long long>(numeric_index));
    return true;
}

}  // namespace

void Interpreter::SetEntryFilePath(const std::string& file_path) {
    entry_file_path_ = std::filesystem::path(file_path);
}

bool Interpreter::Execute(const frontend::Program& program, std::string* out_error) {
    functions_.clear();
    loaded_module_programs_.clear();
    return_stack_.clear();
    module_base_dirs_.clear();
    importing_modules_.clear();

    if (!entry_file_path_.empty()) {
        module_base_dirs_.push_back(entry_file_path_.parent_path());
    }

    for (const auto& statement : program.statements) {
        if (!ExecuteStatement(*statement, out_error)) {
            return false;
        }
    }

    if (!return_stack_.empty()) {
        *out_error = "Error interno: pila de retorno inconsistente.";
        return false;
    }

    return true;
}

bool Interpreter::ExecuteBlock(
    const std::vector<std::unique_ptr<frontend::Statement>>& statements,
    std::string* out_error) {
    for (const auto& statement : statements) {
        if (!ExecuteStatement(*statement, out_error)) {
            return false;
        }

        if (!return_stack_.empty() && return_stack_.back().has_value()) {
            return true;
        }
    }

    return true;
}

bool Interpreter::ExecuteStatement(const frontend::Statement& statement, std::string* out_error) {
    if (const auto* assignment = dynamic_cast<const frontend::AssignmentStmt*>(&statement)) {
        runtime::Value value;
        if (!EvaluateExpression(*assignment->expr, &value, out_error)) {
            return false;
        }
        if (assignment->name.find('.') != std::string::npos) {
            frontend::VariableExpr target(assignment->name);
            return ApplyTargetMutation(target, assignment->op, value, out_error);
        }

        if (assignment->op != frontend::AssignmentOp::Set) {
            return ApplyVariableMutation(assignment->name, assignment->op, value, out_error);
        }

        return AssignValue(*assignment, value, out_error);
    }

    if (const auto* mutation = dynamic_cast<const frontend::MutationStmt*>(&statement)) {
        return ExecuteMutation(*mutation, out_error);
    }

    if (const auto* print = dynamic_cast<const frontend::PrintStmt*>(&statement)) {
        runtime::Value value;
        if (!EvaluateExpression(*print->expr, &value, out_error)) {
            return false;
        }

        std::cout << value.ToString() << std::endl;
        return true;
    }

    if (const auto* conditional = dynamic_cast<const frontend::IfStmt*>(&statement)) {
        runtime::Value condition;
        if (!EvaluateExpression(*conditional->condition, &condition, out_error)) {
            return false;
        }

        if (condition.AsBool()) {
            return ExecuteBlock(conditional->then_branch, out_error);
        }

        return ExecuteBlock(conditional->else_branch, out_error);
    }

    if (const auto* declaration = dynamic_cast<const frontend::FunctionDeclStmt*>(&statement)) {
        functions_[declaration->name] = declaration;
        return true;
    }

    if (const auto* import_stmt = dynamic_cast<const frontend::ImportStmt*>(&statement)) {
        return ImportModule(import_stmt->module_name, out_error);
    }

    if (const auto* expression_stmt = dynamic_cast<const frontend::ExpressionStmt*>(&statement)) {
        if (const auto* call = dynamic_cast<const frontend::CallExpr*>(expression_stmt->expr.get())) {
            runtime::Value ignored;
            return ExecuteCall(*call, false, &ignored, out_error);
        }

        runtime::Value ignored;
        return EvaluateExpression(*expression_stmt->expr, &ignored, out_error);
    }

    if (const auto* return_stmt = dynamic_cast<const frontend::ReturnStmt*>(&statement)) {
        return ExecuteReturn(*return_stmt, out_error);
    }

    *out_error = "Tipo de sentencia no soportado por el interprete.";
    return false;
}

bool Interpreter::ExecuteMutation(const frontend::MutationStmt& statement, std::string* out_error) {
    runtime::Value value;
    if (!EvaluateExpression(*statement.expr, &value, out_error)) {
        return false;
    }

    if (const auto* variable = dynamic_cast<const frontend::VariableExpr*>(statement.target.get())) {
        if (variable->name.find('.') == std::string::npos) {
            return ApplyVariableMutation(variable->name, statement.op, value, out_error);
        }
    }

    return ApplyTargetMutation(*statement.target, statement.op, value, out_error);
}

bool Interpreter::ExecuteReturn(const frontend::ReturnStmt& statement, std::string* out_error) {
    if (return_stack_.empty()) {
        *out_error = "return solo se permite dentro de una funcion.";
        return false;
    }

    runtime::Value value(0.0);
    if (statement.expr != nullptr) {
        if (!EvaluateExpression(*statement.expr, &value, out_error)) {
            return false;
        }
    }

    return_stack_.back() = std::move(value);
    return true;
}

bool Interpreter::EvaluateExpression(
    const frontend::Expr& expression,
    runtime::Value* out_value,
    std::string* out_error) {
    if (const auto* number = dynamic_cast<const frontend::NumberExpr*>(&expression)) {
        if (number->exact_integer.has_value()) {
            *out_value = runtime::Value(*number->exact_integer);
            return true;
        }
        *out_value = runtime::Value(number->value);
        return true;
    }

    if (const auto* text = dynamic_cast<const frontend::StringExpr*>(&expression)) {
        *out_value = runtime::Value(text->value);
        return true;
    }

    if (const auto* boolean = dynamic_cast<const frontend::BoolExpr*>(&expression)) {
        *out_value = runtime::Value(boolean->value);
        return true;
    }

    if (const auto* variable = dynamic_cast<const frontend::VariableExpr*>(&expression)) {
        return ResolveVariable(variable->name, out_value, out_error);
    }

    if (const auto* list = dynamic_cast<const frontend::ListExpr*>(&expression)) {
        runtime::Value::List values;
        values.reserve(list->elements.size());
        for (const auto& element : list->elements) {
            runtime::Value evaluated;
            if (!EvaluateExpression(*element, &evaluated, out_error)) {
                return false;
            }
            values.push_back(std::move(evaluated));
        }

        *out_value = runtime::Value(std::move(values));
        return true;
    }

    if (const auto* object = dynamic_cast<const frontend::ObjectExpr*>(&expression)) {
        runtime::Value::Object values;
        values.reserve(object->entries.size());
        for (const auto& entry : object->entries) {
            runtime::Value evaluated;
            if (!EvaluateExpression(*entry.value, &evaluated, out_error)) {
                return false;
            }
            values.push_back({entry.key, std::move(evaluated)});
        }

        *out_value = runtime::Value(std::move(values));
        return true;
    }

    if (const auto* index = dynamic_cast<const frontend::IndexExpr*>(&expression)) {
        runtime::Value collection;
        runtime::Value index_value;

        if (!EvaluateExpression(*index->collection, &collection, out_error)) {
            return false;
        }
        if (!EvaluateExpression(*index->index, &index_value, out_error)) {
            return false;
        }

        const runtime::Value::List* list = collection.AsList();
        if (list == nullptr) {
            *out_error = "Solo se puede indexar una lista con [].";
            return false;
        }

        std::size_t integer_index = 0;
        if (!ReadListIndex(index_value, &integer_index, out_error)) {
            return false;
        }

        if (integer_index >= list->size()) {
            *out_error = "Indice fuera de rango en lista.";
            return false;
        }

        *out_value = (*list)[integer_index];
        return true;
    }

    if (const auto* call = dynamic_cast<const frontend::CallExpr*>(&expression)) {
        return ExecuteCall(*call, true, out_value, out_error);
    }

    if (const auto* unary = dynamic_cast<const frontend::UnaryExpr*>(&expression)) {
        runtime::Value operand;
        if (!EvaluateExpression(*unary->operand, &operand, out_error)) {
            return false;
        }

        return EvaluateUnary(unary->op, operand, out_value, out_error);
    }

    if (const auto* binary = dynamic_cast<const frontend::BinaryExpr*>(&expression)) {
        runtime::Value lhs;
        runtime::Value rhs;

        if (!EvaluateExpression(*binary->lhs, &lhs, out_error)) {
            return false;
        }
        if (!EvaluateExpression(*binary->rhs, &rhs, out_error)) {
            return false;
        }

        return EvaluateBinary(binary->op, lhs, rhs, out_value, out_error);
    }

    *out_error = "Expresion no soportada por el interprete.";
    return false;
}

bool Interpreter::EvaluateUnary(
    frontend::UnaryOp op,
    const runtime::Value& operand,
    runtime::Value* out_value,
    std::string* out_error) const {
    if (op == frontend::UnaryOp::LogicalNot) {
        *out_value = runtime::Value(!operand.AsBool());
        return true;
    }

    long long integer = 0;
    if (ReadInteger(operand, &integer)) {
        if (op == frontend::UnaryOp::Negate) {
            if (integer == std::numeric_limits<long long>::min()) {
                *out_value = runtime::Value(-static_cast<double>(integer));
            } else {
                *out_value = runtime::Value(-integer);
            }
            return true;
        }

        *out_value = runtime::Value(integer);
        return true;
    }

    double numeric = 0.0;
    if (!ReadNumeric(operand, &numeric, out_error)) {
        return false;
    }

    if (op == frontend::UnaryOp::Negate) {
        *out_value = runtime::Value(-numeric);
        return true;
    }

    *out_value = runtime::Value(numeric);
    return true;
}

bool Interpreter::EvaluateBinary(
    frontend::BinaryOp op,
    const runtime::Value& lhs,
    const runtime::Value& rhs,
    runtime::Value* out_value,
    std::string* out_error) const {
    if (op == frontend::BinaryOp::Add && (lhs.IsString() || rhs.IsString())) {
        *out_value = runtime::Value(lhs.ToString() + rhs.ToString());
        return true;
    }

    if (op == frontend::BinaryOp::LogicalAnd) {
        *out_value = runtime::Value(lhs.AsBool() && rhs.AsBool());
        return true;
    }

    if (op == frontend::BinaryOp::LogicalOr) {
        *out_value = runtime::Value(lhs.AsBool() || rhs.AsBool());
        return true;
    }

    long long left_integer = 0;
    long long right_integer = 0;
    const bool left_is_integer = ReadInteger(lhs, &left_integer);
    const bool right_is_integer = ReadInteger(rhs, &right_integer);

    if (op == frontend::BinaryOp::Equal || op == frontend::BinaryOp::NotEqual) {
        bool result = false;

        if (lhs.IsList() || lhs.IsObject() || rhs.IsList() || rhs.IsObject()) {
            result = lhs.ToString() == rhs.ToString();
        } else if (lhs.IsString() || rhs.IsString()) {
            result = lhs.ToString() == rhs.ToString();
        } else if (lhs.IsBool() || rhs.IsBool()) {
            result = lhs.AsBool() == rhs.AsBool();
        } else if (left_is_integer && right_is_integer) {
            result = left_integer == right_integer;
        } else {
            double left_number = 0.0;
            double right_number = 0.0;
            if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
                return false;
            }
            result = left_number == right_number;
        }

        *out_value = runtime::Value(op == frontend::BinaryOp::Equal ? result : !result);
        return true;
    }

    if (left_is_integer && right_is_integer) {
        switch (op) {
        case frontend::BinaryOp::Add: {
            long long result = 0;
            if (CheckedAdd(left_integer, right_integer, &result)) {
                *out_value = runtime::Value(result);
            } else {
                *out_value = runtime::Value(static_cast<double>(left_integer) + static_cast<double>(right_integer));
            }
            return true;
        }
        case frontend::BinaryOp::Subtract: {
            long long result = 0;
            if (CheckedSubtract(left_integer, right_integer, &result)) {
                *out_value = runtime::Value(result);
            } else {
                *out_value = runtime::Value(static_cast<double>(left_integer) - static_cast<double>(right_integer));
            }
            return true;
        }
        case frontend::BinaryOp::Less:
            *out_value = runtime::Value(left_integer < right_integer);
            return true;
        case frontend::BinaryOp::LessEqual:
            *out_value = runtime::Value(left_integer <= right_integer);
            return true;
        case frontend::BinaryOp::Greater:
            *out_value = runtime::Value(left_integer > right_integer);
            return true;
        case frontend::BinaryOp::GreaterEqual:
            *out_value = runtime::Value(left_integer >= right_integer);
            return true;
        case frontend::BinaryOp::Multiply:
        case frontend::BinaryOp::Divide:
        case frontend::BinaryOp::Modulo:
        case frontend::BinaryOp::Power:
        case frontend::BinaryOp::Equal:
        case frontend::BinaryOp::NotEqual:
        case frontend::BinaryOp::LogicalAnd:
        case frontend::BinaryOp::LogicalOr:
            break;
        }
    }

    double left_number = 0.0;
    double right_number = 0.0;
    if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
        return false;
    }

    switch (op) {
    case frontend::BinaryOp::Add:
        *out_value = runtime::Value(left_number + right_number);
        return true;
    case frontend::BinaryOp::Subtract:
        *out_value = runtime::Value(left_number - right_number);
        return true;
    case frontend::BinaryOp::Multiply:
        *out_value = runtime::Value(left_number * right_number);
        return true;
    case frontend::BinaryOp::Divide:
        *out_value = runtime::Value(left_number / right_number);
        return true;
    case frontend::BinaryOp::Modulo:
        *out_value = runtime::Value(std::fmod(left_number, right_number));
        return true;
    case frontend::BinaryOp::Power:
        *out_value = runtime::Value(std::pow(left_number, right_number));
        return true;
    case frontend::BinaryOp::Less:
        *out_value = runtime::Value(left_number < right_number);
        return true;
    case frontend::BinaryOp::LessEqual:
        *out_value = runtime::Value(left_number <= right_number);
        return true;
    case frontend::BinaryOp::Greater:
        *out_value = runtime::Value(left_number > right_number);
        return true;
    case frontend::BinaryOp::GreaterEqual:
        *out_value = runtime::Value(left_number >= right_number);
        return true;
    case frontend::BinaryOp::Equal:
    case frontend::BinaryOp::NotEqual:
    case frontend::BinaryOp::LogicalAnd:
    case frontend::BinaryOp::LogicalOr:
        break;
    }

    *out_error = "Operacion binaria no soportada.";
    return false;
}

bool Interpreter::ExecuteCall(
    const frontend::CallExpr& call,
    bool require_return_value,
    runtime::Value* out_value,
    std::string* out_error) {
    bool was_builtin = false;
    if (!ExecuteBuiltinCall(call, &was_builtin, out_value, out_error)) {
        return false;
    }
    if (was_builtin) {
        return true;
    }

    const auto function_it = functions_.find(call.callee);
    if (function_it == functions_.end()) {
        *out_error = "Funcion no definida: " + call.callee;
        return false;
    }

    return ExecuteUserFunction(*function_it->second, call, require_return_value, out_value, out_error);
}

bool Interpreter::ExecuteBuiltinCall(
    const frontend::CallExpr& call,
    bool* out_was_builtin,
    runtime::Value* out_value,
    std::string* out_error) {
    *out_was_builtin = false;

    if (call.callee == "sum" && imported_modules_.count("math") > 0) {
        *out_was_builtin = true;

        if (call.arguments.size() != 2) {
            *out_error = "sum(a, b) requiere 2 argumentos.";
            return false;
        }

        runtime::Value left;
        runtime::Value right;

        if (!EvaluateExpression(*call.arguments[0].value, &left, out_error)) {
            return false;
        }
        if (!EvaluateExpression(*call.arguments[1].value, &right, out_error)) {
            return false;
        }

        double left_number = 0.0;
        double right_number = 0.0;
        if (!ReadNumeric(left, &left_number, out_error) || !ReadNumeric(right, &right_number, out_error)) {
            return false;
        }

        *out_value = runtime::Value(left_number + right_number);
        return true;
    }

    return true;
}

bool Interpreter::ExecuteUserFunction(
    const frontend::FunctionDeclStmt& function,
    const frontend::CallExpr& call,
    bool require_return_value,
    runtime::Value* out_value,
    std::string* out_error) {
    if (call.arguments.size() != function.params.size()) {
        *out_error = "Numero incorrecto de argumentos para funcion '" + function.name + "'.";
        return false;
    }

    struct RefBinding {
        std::string param;
        std::string caller;
    };

    std::vector<RefBinding> refs;
    auto caller_environment = environment_;
    auto local_environment = environment_;

    for (std::size_t i = 0; i < function.params.size(); ++i) {
        const frontend::FunctionParam& param = function.params[i];
        const frontend::CallArgument& argument = call.arguments[i];

        if (param.by_reference) {
            const auto* variable = dynamic_cast<const frontend::VariableExpr*>(argument.value.get());
            if (variable == nullptr) {
                *out_error = "Parametro por referencia '" + param.name + "' requiere una variable.";
                return false;
            }

            if (variable->name.find('.') != std::string::npos) {
                *out_error = "Referencia no soporta acceso con propiedad: " + variable->name;
                return false;
            }

            const auto caller_it = caller_environment.find(variable->name);
            if (caller_it == caller_environment.end()) {
                *out_error = "Variable no definida para referencia: " + variable->name;
                return false;
            }

            local_environment[param.name] = caller_it->second;
            refs.push_back(RefBinding{param.name, variable->name});
            continue;
        }

        if (argument.by_reference) {
            *out_error = "No se puede pasar '&' a un parametro por valor: " + param.name;
            return false;
        }

        runtime::Value evaluated;
        if (!EvaluateExpression(*argument.value, &evaluated, out_error)) {
            return false;
        }

        local_environment[param.name] = runtime::VariableSlot{evaluated, runtime::VariableKind::Dynamic};
    }

    environment_ = std::move(local_environment);
    return_stack_.push_back(std::nullopt);

    if (!ExecuteBlock(function.body, out_error)) {
        return_stack_.pop_back();
        environment_ = std::move(caller_environment);
        return false;
    }

    std::optional<runtime::Value> returned = return_stack_.back();
    return_stack_.pop_back();

    for (const RefBinding& ref : refs) {
        const auto updated = environment_.find(ref.param);
        if (updated != environment_.end()) {
            caller_environment[ref.caller] = updated->second;
        }
    }

    environment_ = std::move(caller_environment);

    if (require_return_value) {
        if (!returned.has_value()) {
            *out_error = "La funcion '" + function.name + "' no retorno ningun valor.";
            return false;
        }

        if (out_value != nullptr) {
            *out_value = *returned;
        }
        return true;
    }

    if (out_value != nullptr) {
        *out_value = returned.has_value() ? *returned : runtime::Value(0.0);
    }

    return true;
}

}  // namespace clot::interpreter
