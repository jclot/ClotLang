#include "clot/interpreter/interpreter.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>

#include "clot/runtime/i18n.hpp"

namespace clot::interpreter {

namespace {

using BigInt = runtime::Value::BigInt;

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

bool TryBigIntToSizeT(const BigInt& value, std::size_t* out_index) {
    if (out_index == nullptr || value < 0) {
        return false;
    }

    static const BigInt kMaxSizeT = [] {
        BigInt parsed;
        (void)BigInt::TryParse(std::to_string(std::numeric_limits<std::size_t>::max()), &parsed);
        return parsed;
    }();
    if (value > kMaxSizeT) {
        return false;
    }

    *out_index = value.convert_to<std::size_t>();
    return true;
}

bool PowBigInt(const BigInt& base, const BigInt& exponent, BigInt* out_result, std::string* out_error) {
    if (out_result == nullptr) {
        if (out_error != nullptr) {
            *out_error = "Error interno: salida nula en potencia entera.";
        }
        return false;
    }

    if (exponent < 0) {
        if (out_error != nullptr) {
            *out_error = "pow() no acepta exponente entero negativo para resultado entero.";
        }
        return false;
    }

    long long exponent_i64 = 0;
    if (!runtime::Value::TryBigIntToInt64(exponent, &exponent_i64)) {
        if (out_error != nullptr) {
            *out_error = "pow() exponente entero demasiado grande para computar.";
        }
        return false;
    }
    if (exponent_i64 < 0) {
        if (out_error != nullptr) {
            *out_error = "pow() no acepta exponente entero negativo para resultado entero.";
        }
        return false;
    }
    if (exponent_i64 > 1000000LL) {
        if (out_error != nullptr) {
            *out_error = "pow() exponente entero demasiado grande para computar.";
        }
        return false;
    }

    BigInt power = base;
    BigInt result = 1;
    unsigned long long exp = static_cast<unsigned long long>(exponent_i64);
    while (exp > 0) {
        if ((exp & 1ULL) != 0ULL) {
            result *= power;
        }
        exp >>= 1ULL;
        if (exp != 0ULL) {
            power *= power;
        }
    }

    *out_result = std::move(result);
    return true;
}

bool ReadListIndex(const runtime::Value& value, std::size_t* out_index, std::string* out_error) {
    BigInt integer_index;
    if (!value.AsBigInt(&integer_index)) {
        if (out_error != nullptr) {
            *out_error = "El indice de lista debe ser un entero finito.";
        }
        return false;
    }

    if (!TryBigIntToSizeT(integer_index, out_index)) {
        if (out_error != nullptr) {
            *out_error = "El indice de lista debe ser un entero finito.";
        }
        return false;
    }
    return true;
}

} // namespace

void Interpreter::SetEntryFilePath(const std::string& file_path) {
    entry_file_path_ = std::filesystem::path(file_path);
}

bool Interpreter::Execute(const frontend::Program& program, std::string* out_error) {
    environment_.clear();
    functions_.clear();
    imported_modules_.clear();
    loaded_module_programs_.clear();
    return_stack_.clear();
    module_base_dirs_.clear();
    importing_modules_.clear();
    async_tasks_.clear();
    next_async_task_id_ = 1;

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

bool Interpreter::ExecuteBlock(const std::vector<std::unique_ptr<frontend::Statement>>& statements,
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
        bool has_value = false;

        if (assignment->declaration_type == frontend::DeclarationType::Function &&
            assignment->op == frontend::AssignmentOp::Set) {
            const auto* call_expr = dynamic_cast<const frontend::CallExpr*>(assignment->expr.get());
            if (call_expr != nullptr && call_expr->arguments.empty()) {
                const auto function_decl = functions_.find(call_expr->callee);
                if (function_decl != functions_.end()) {
                    value = runtime::Value(runtime::Value::FunctionRef{call_expr->callee});
                    has_value = true;
                } else {
                    const auto function_var = environment_.find(call_expr->callee);
                    if (function_var != environment_.end()) {
                        const auto* function_ref = function_var->second.value.AsFunctionRefValue();
                        if (function_ref != nullptr) {
                            value = runtime::Value(*function_ref);
                            has_value = true;
                        }
                    }
                }
            }
        }

        if (!has_value) {
            if (!EvaluateExpression(*assignment->expr, &value, out_error)) {
                return false;
            }
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
        if (print->expr != nullptr) {
            runtime::Value value;
            if (!EvaluateExpression(*print->expr, &value, out_error)) {
                return false;
            }
            std::cout << value.ToString();
        }

        if (print->append_newline) {
            std::cout << std::endl;
        } else {
            std::cout << std::flush;
        }
        return true;
    }

    if (const auto* while_stmt = dynamic_cast<const frontend::WhileStmt*>(&statement)) {
        while (true) {
            runtime::Value condition;
            if (!EvaluateExpression(*while_stmt->condition, &condition, out_error))
                return false;
            if (!condition.AsBool())
                break;

            if (!ExecuteBlock(while_stmt->body, out_error))
                return false;

            if (!return_stack_.empty() && return_stack_.back().has_value())
                return true;
        }
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

    if (const auto* enum_decl = dynamic_cast<const frontend::EnumDeclStmt*>(&statement)) {
        runtime::Value::Object values;
        values.reserve(enum_decl->members.size());
        runtime::Value::Map value_to_name;
        value_to_name.entries.reserve(enum_decl->members.size());
        runtime::Value::Map name_to_value;
        name_to_value.entries.reserve(enum_decl->members.size());

        std::set<std::string> used_names;
        long long next_value = 0;
        for (const auto& member : enum_decl->members) {
            if (used_names.count(member) > 0) {
                *out_error = "Miembro enum duplicado: " + member;
                return false;
            }
            used_names.insert(member);
            values.push_back({member, runtime::Value(next_value)});
            value_to_name.entries.push_back({runtime::Value(next_value), runtime::Value(member)});
            name_to_value.entries.push_back({runtime::Value(member), runtime::Value(next_value)});
            ++next_value;
        }

        values.push_back({"__value_to_name", runtime::Value(std::move(value_to_name))});
        values.push_back({"__name_to_value", runtime::Value(std::move(name_to_value))});

        environment_[enum_decl->name] =
            runtime::VariableSlot{runtime::Value(std::move(values)), runtime::VariableKind::Dynamic};
        return true;
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

    if (const auto* try_catch_stmt = dynamic_cast<const frontend::TryCatchStmt*>(&statement)) {
        return ExecuteTryCatch(*try_catch_stmt, out_error);
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

    runtime::Value value(nullptr);
    if (statement.expr != nullptr) {
        if (!EvaluateExpression(*statement.expr, &value, out_error)) {
            return false;
        }
    }

    return_stack_.back() = std::move(value);
    return true;
}

bool Interpreter::ExecuteTryCatch(const frontend::TryCatchStmt& statement, std::string* out_error) {
    std::string try_error;
    if (ExecuteBlock(statement.try_branch, &try_error)) {
        return true;
    }

    const bool has_active_return = !return_stack_.empty() && return_stack_.back().has_value();
    if (has_active_return) {
        if (out_error != nullptr) {
            *out_error = try_error;
        }
        return false;
    }

    std::optional<runtime::VariableSlot> previous_slot;
    if (!statement.error_binding.empty()) {
        const auto existing = environment_.find(statement.error_binding);
        if (existing != environment_.end()) {
            previous_slot = existing->second;
        }

        const std::string localized_try_error = runtime::TranslateDiagnostic(try_error);
        environment_[statement.error_binding] = runtime::VariableSlot{
            runtime::Value(localized_try_error),
            runtime::VariableKind::Dynamic,
        };
    }

    std::string catch_error;
    const bool catch_ok = ExecuteBlock(statement.catch_branch, &catch_error);

    if (!statement.error_binding.empty()) {
        if (previous_slot.has_value()) {
            environment_[statement.error_binding] = *previous_slot;
        } else {
            environment_.erase(statement.error_binding);
        }
    }

    if (!catch_ok) {
        if (out_error != nullptr) {
            *out_error = catch_error;
        }
        return false;
    }

    return true;
}

bool Interpreter::EvaluateExpression(const frontend::Expr& expression, runtime::Value* out_value,
                                     std::string* out_error) {
    if (const auto* number = dynamic_cast<const frontend::NumberExpr*>(&expression)) {
        if (number->is_integer_literal) {
            runtime::Value::BigInt integer;
            if (!runtime::Value::TryParseBigInt(number->lexeme, &integer)) {
                *out_error = "Numero invalido: '" + number->lexeme + "'.";
                return false;
            }
            *out_value = runtime::Value(std::move(integer));
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

    if (const auto* character = dynamic_cast<const frontend::CharExpr*>(&expression)) {
        *out_value = runtime::Value(character->value);
        return true;
    }

    if (dynamic_cast<const frontend::NullExpr*>(&expression) != nullptr) {
        *out_value = runtime::Value(nullptr);
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

        if (const runtime::Value::List* list = collection.AsList()) {
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

        if (const std::vector<runtime::Value>* tuple = collection.AsTuple()) {
            std::size_t integer_index = 0;
            if (!ReadListIndex(index_value, &integer_index, out_error)) {
                return false;
            }

            if (integer_index >= tuple->size()) {
                *out_error = "Indice fuera de rango en tuple.";
                return false;
            }

            *out_value = (*tuple)[integer_index];
            return true;
        }

        if (const auto* map = collection.AsMap()) {
            for (const auto& entry : *map) {
                if (entry.first.Equals(index_value)) {
                    *out_value = entry.second;
                    return true;
                }
            }
            *out_error = "Clave no encontrada en map.";
            return false;
        }

        if (const auto* object = collection.AsObject()) {
            const std::string key = index_value.ToString();
            for (const auto& entry : *object) {
                if (entry.first == key) {
                    *out_value = entry.second;
                    return true;
                }
            }
            *out_error = "Propiedad no encontrada: " + key;
            return false;
        }

        *out_error = "Solo se puede indexar list, tuple, map u object con [].";
        return false;
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

bool Interpreter::EvaluateUnary(frontend::UnaryOp op, const runtime::Value& operand, runtime::Value* out_value,
                                std::string* out_error) const {
    if (op == frontend::UnaryOp::LogicalNot) {
        *out_value = runtime::Value(!operand.AsBool());
        return true;
    }

    BigInt integer;
    if (operand.AsBigInt(&integer)) {
        if (op == frontend::UnaryOp::Negate) {
            *out_value = runtime::Value(-integer);
            return true;
        }

        *out_value = runtime::Value(std::move(integer));
        return true;
    }

    if (operand.IsDecimal()) {
        runtime::Value::Decimal decimal;
        if (!operand.AsDecimal(&decimal)) {
            *out_error = "La expresion requiere un valor numerico.";
            return false;
        }

        if (op == frontend::UnaryOp::Negate) {
            *out_value = runtime::Value(runtime::Value::Decimal::FromBigInt(0) - decimal);
            return true;
        }

        *out_value = runtime::Value(decimal);
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

bool Interpreter::EvaluateBinary(frontend::BinaryOp op, const runtime::Value& lhs, const runtime::Value& rhs,
                                 runtime::Value* out_value, std::string* out_error) const {
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

    if (op == frontend::BinaryOp::Equal || op == frontend::BinaryOp::NotEqual) {
        const bool result = lhs.Equals(rhs);
        *out_value = runtime::Value(op == frontend::BinaryOp::Equal ? result : !result);
        return true;
    }

    runtime::Value::Decimal left_decimal;
    runtime::Value::Decimal right_decimal;
    const bool use_decimal =
        (lhs.IsDecimal() || rhs.IsDecimal()) &&
        lhs.AsDecimal(&left_decimal) &&
        rhs.AsDecimal(&right_decimal);

    if (use_decimal) {
        switch (op) {
        case frontend::BinaryOp::Add:
            *out_value = runtime::Value(left_decimal + right_decimal);
            return true;
        case frontend::BinaryOp::Subtract:
            *out_value = runtime::Value(left_decimal - right_decimal);
            return true;
        case frontend::BinaryOp::Multiply:
            *out_value = runtime::Value(left_decimal * right_decimal);
            return true;
        case frontend::BinaryOp::Divide: {
            runtime::Value::Decimal divided;
            if (!runtime::Value::Decimal::Divide(left_decimal, right_decimal, 18, &divided, out_error)) {
                return false;
            }
            *out_value = runtime::Value(std::move(divided));
            return true;
        }
        case frontend::BinaryOp::Power: {
            BigInt exponent_integer;
            if (rhs.AsBigInt(&exponent_integer) && exponent_integer >= 0) {
                long long exponent = 0;
                if (!runtime::Value::TryBigIntToInt64(exponent_integer, &exponent) || exponent > 1000000LL) {
                    *out_error = "pow() exponente entero demasiado grande para computar.";
                    return false;
                }

                runtime::Value::Decimal result = runtime::Value::Decimal::FromBigInt(1);
                runtime::Value::Decimal power = left_decimal;
                unsigned long long exp = static_cast<unsigned long long>(exponent);
                while (exp > 0) {
                    if ((exp & 1ULL) != 0ULL) {
                        result = result * power;
                    }
                    exp >>= 1ULL;
                    if (exp != 0ULL) {
                        power = power * power;
                    }
                }
                *out_value = runtime::Value(std::move(result));
                return true;
            }

            double left_number = 0.0;
            double right_number = 0.0;
            if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
                return false;
            }
            *out_value = runtime::Value(std::pow(left_number, right_number));
            return true;
        }
        case frontend::BinaryOp::Modulo: {
            double left_number = 0.0;
            double right_number = 0.0;
            if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
                return false;
            }
            if (right_number == 0.0) {
                *out_error = "Modulo por cero.";
                return false;
            }
            *out_value = runtime::Value(std::fmod(left_number, right_number));
            return true;
        }
        case frontend::BinaryOp::Less:
            *out_value = runtime::Value(left_decimal < right_decimal);
            return true;
        case frontend::BinaryOp::LessEqual:
            *out_value = runtime::Value(left_decimal <= right_decimal);
            return true;
        case frontend::BinaryOp::Greater:
            *out_value = runtime::Value(left_decimal > right_decimal);
            return true;
        case frontend::BinaryOp::GreaterEqual:
            *out_value = runtime::Value(left_decimal >= right_decimal);
            return true;
        case frontend::BinaryOp::Equal:
        case frontend::BinaryOp::NotEqual:
        case frontend::BinaryOp::LogicalAnd:
        case frontend::BinaryOp::LogicalOr:
            break;
        }
    }

    BigInt left_integer;
    BigInt right_integer;
    const bool left_is_integer = lhs.AsBigInt(&left_integer);
    const bool right_is_integer = rhs.AsBigInt(&right_integer);

    if (left_is_integer && right_is_integer) {
        switch (op) {
        case frontend::BinaryOp::Add:
            *out_value = runtime::Value(left_integer + right_integer);
            return true;
        case frontend::BinaryOp::Subtract:
            *out_value = runtime::Value(left_integer - right_integer);
            return true;
        case frontend::BinaryOp::Multiply:
            *out_value = runtime::Value(left_integer * right_integer);
            return true;
        case frontend::BinaryOp::Divide: {
            if (right_integer == 0) {
                *out_error = "Division por cero.";
                return false;
            }

            double left_number = 0.0;
            double right_number = 0.0;
            if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
                *out_error = "No se puede convertir entero grande a double para division.";
                return false;
            }
            *out_value = runtime::Value(left_number / right_number);
            return true;
        }
        case frontend::BinaryOp::Modulo:
            if (right_integer == 0) {
                *out_error = "Modulo por cero.";
                return false;
            }
            *out_value = runtime::Value(left_integer % right_integer);
            return true;
        case frontend::BinaryOp::Power: {
            if (right_integer < 0) {
                double left_number = 0.0;
                double right_number = 0.0;
                if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
                    *out_error = "No se puede convertir entero grande a double para potencia.";
                    return false;
                }
                *out_value = runtime::Value(std::pow(left_number, right_number));
                return true;
            }

            BigInt result;
            if (!PowBigInt(left_integer, right_integer, &result, out_error)) {
                return false;
            }
            *out_value = runtime::Value(std::move(result));
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
        if (right_number == 0.0) {
            *out_error = "Division por cero.";
            return false;
        }
        *out_value = runtime::Value(left_number / right_number);
        return true;
    case frontend::BinaryOp::Modulo:
        if (right_number == 0.0) {
            *out_error = "Modulo por cero.";
            return false;
        }
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

bool Interpreter::ExecuteCall(const frontend::CallExpr& call, bool require_return_value, runtime::Value* out_value,
                              std::string* out_error) {
    bool was_builtin = false;
    if (!ExecuteBuiltinCall(call, &was_builtin, out_value, out_error)) {
        return false;
    }
    if (was_builtin) {
        return true;
    }

    std::string target_function = call.callee;
    const auto variable_it = environment_.find(call.callee);
    if (variable_it != environment_.end()) {
        const auto* function_ref = variable_it->second.value.AsFunctionRefValue();
        if (function_ref != nullptr && !function_ref->name.empty()) {
            target_function = function_ref->name;
        }
    }

    const auto function_it = functions_.find(target_function);
    if (function_it == functions_.end()) {
        *out_error = "Funcion no definida: " + target_function;
        return false;
    }

    return ExecuteUserFunction(*function_it->second, call, require_return_value, out_value, out_error);
}

bool Interpreter::ExecuteUserFunction(const frontend::FunctionDeclStmt& function, const frontend::CallExpr& call,
                                      bool require_return_value, runtime::Value* out_value, std::string* out_error) {
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
        if (out_value != nullptr) {
            *out_value = returned.has_value() ? *returned : runtime::Value(nullptr);
        }
        return true;
    }

    if (out_value != nullptr) {
        *out_value = returned.has_value() ? *returned : runtime::Value(nullptr);
    }

    return true;
}

} // namespace clot::interpreter
