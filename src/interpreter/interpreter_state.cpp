#include "clot/interpreter/interpreter.hpp"

#include <limits>
#include <string>
#include <utility>

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

}  // namespace

bool Interpreter::ResolveVariable(
    const std::string& name,
    runtime::Value* out_value,
    std::string* out_error) const {
    const std::size_t dot = name.find('.');
    if (dot == std::string::npos) {
        const auto found = environment_.find(name);
        if (found == environment_.end()) {
            *out_error = "Variable no definida: " + name;
            return false;
        }

        *out_value = found->second.value;
        return true;
    }

    const std::string root_name = name.substr(0, dot);
    const auto root = environment_.find(root_name);
    if (root == environment_.end()) {
        *out_error = "Variable no definida: " + root_name;
        return false;
    }

    const runtime::Value* current = &root->second.value;
    std::size_t segment_start = dot + 1;
    while (segment_start <= name.size()) {
        const std::size_t next_dot = name.find('.', segment_start);
        const std::string segment = name.substr(
            segment_start,
            next_dot == std::string::npos ? std::string::npos : next_dot - segment_start);

        if (segment.empty()) {
            *out_error = "Acceso de propiedad invalido: " + name;
            return false;
        }

        const runtime::Value* nested = current->GetObjectProperty(segment);
        if (nested == nullptr) {
            *out_error = "Propiedad no encontrada: " + segment;
            return false;
        }

        current = nested;
        if (next_dot == std::string::npos) {
            break;
        }

        segment_start = next_dot + 1;
    }

    *out_value = *current;
    return true;
}

bool Interpreter::ResolveMutableVariable(
    const std::string& name,
    bool create_missing_property,
    runtime::Value** out_value,
    std::string* out_error) {
    if (out_value == nullptr) {
        if (out_error != nullptr) {
            *out_error = "Error interno: out_value nulo en ResolveMutableVariable.";
        }
        return false;
    }

    const std::size_t dot = name.find('.');
    if (dot == std::string::npos) {
        const auto found = environment_.find(name);
        if (found == environment_.end()) {
            *out_error = "Variable no definida: " + name;
            return false;
        }

        *out_value = &found->second.value;
        return true;
    }

    const std::string root_name = name.substr(0, dot);
    const auto root = environment_.find(root_name);
    if (root == environment_.end()) {
        *out_error = "Variable no definida: " + root_name;
        return false;
    }

    runtime::Value* current = &root->second.value;
    std::size_t segment_start = dot + 1;
    while (segment_start <= name.size()) {
        const std::size_t next_dot = name.find('.', segment_start);
        const bool is_last_segment = next_dot == std::string::npos;
        const std::string segment = name.substr(
            segment_start,
            is_last_segment ? std::string::npos : next_dot - segment_start);

        if (segment.empty()) {
            *out_error = "Acceso de propiedad invalido: " + name;
            return false;
        }

        runtime::Value* nested = nullptr;
        if (create_missing_property && is_last_segment) {
            nested = current->EnsureObjectProperty(segment);
        } else {
            nested = current->GetMutableObjectProperty(segment);
        }

        if (nested == nullptr) {
            if (!current->IsObject()) {
                *out_error = "No se puede acceder propiedad en un valor no objeto: " + segment;
            } else {
                *out_error = "Propiedad no encontrada: " + segment;
            }
            return false;
        }

        current = nested;
        if (is_last_segment) {
            break;
        }
        segment_start = next_dot + 1;
    }

    *out_value = current;
    return true;
}

bool Interpreter::ResolveMutableTarget(
    const frontend::Expr& target,
    bool create_missing_property,
    runtime::Value** out_value,
    std::string* out_error) {
    if (const auto* variable = dynamic_cast<const frontend::VariableExpr*>(&target)) {
        return ResolveMutableVariable(variable->name, create_missing_property, out_value, out_error);
    }

    if (const auto* index = dynamic_cast<const frontend::IndexExpr*>(&target)) {
        runtime::Value* collection = nullptr;
        if (!ResolveMutableTarget(*index->collection, false, &collection, out_error)) {
            return false;
        }

        runtime::Value index_value;
        if (!EvaluateExpression(*index->index, &index_value, out_error)) {
            return false;
        }

        runtime::Value::List* list = collection->MutableList();
        if (list == nullptr) {
            *out_error = "Solo se puede mutar una lista con [].";
            return false;
        }

        double numeric_index = 0.0;
        if (!ReadNumeric(index_value, &numeric_index, out_error)) {
            return false;
        }

        const long long integer_index = static_cast<long long>(numeric_index);
        if (integer_index < 0 || static_cast<std::size_t>(integer_index) >= list->size()) {
            *out_error = "Indice fuera de rango en lista.";
            return false;
        }

        *out_value = &(*list)[static_cast<std::size_t>(integer_index)];
        return true;
    }

    *out_error = "El lado izquierdo de una mutacion debe ser variable o indexacion.";
    return false;
}

bool Interpreter::NormalizeValueForKind(
    runtime::VariableKind kind,
    const runtime::Value& value,
    runtime::Value* out_value,
    std::string* out_error) const {
    if (out_value == nullptr) {
        if (out_error != nullptr) {
            *out_error = "Error interno: out_value nulo en NormalizeValueForKind.";
        }
        return false;
    }

    runtime::Value normalized = value;
    if (kind == runtime::VariableKind::Long || kind == runtime::VariableKind::Byte) {
        double numeric = 0.0;
        if (!ReadNumeric(value, &numeric, out_error)) {
            return false;
        }

        const long long integer_value = static_cast<long long>(numeric);
        if (kind == runtime::VariableKind::Long) {
            if (numeric < static_cast<double>(std::numeric_limits<long long>::min()) ||
                numeric > static_cast<double>(std::numeric_limits<long long>::max())) {
                *out_error = "Valor fuera de rango para long.";
                return false;
            }
            normalized = runtime::Value(static_cast<double>(integer_value));
        } else {
            if (numeric < 0.0 || numeric > 255.0) {
                *out_error = "Valor fuera de rango para byte (0-255).";
                return false;
            }
            normalized = runtime::Value(static_cast<double>(static_cast<unsigned char>(integer_value)));
        }
    }

    *out_value = std::move(normalized);
    return true;
}

bool Interpreter::AssignValue(
    const frontend::AssignmentStmt& statement,
    const runtime::Value& value,
    std::string* out_error) {
    if (statement.name.find('.') != std::string::npos) {
        if (statement.declaration_type != frontend::DeclarationType::Inferred) {
            *out_error = "No se puede declarar tipo long/byte sobre una propiedad de objeto.";
            return false;
        }

        frontend::VariableExpr target(statement.name);
        return ApplyTargetMutation(target, statement.op, value, out_error);
    }

    if (statement.op != frontend::AssignmentOp::Set) {
        return ApplyVariableMutation(statement.name, statement.op, value, out_error);
    }

    runtime::VariableKind target_kind = runtime::VariableKind::Dynamic;

    const auto existing = environment_.find(statement.name);
    if (existing != environment_.end()) {
        target_kind = existing->second.kind;
    }

    if (statement.declaration_type == frontend::DeclarationType::Long) {
        target_kind = runtime::VariableKind::Long;
    } else if (statement.declaration_type == frontend::DeclarationType::Byte) {
        target_kind = runtime::VariableKind::Byte;
    }

    runtime::Value normalized;
    if (!NormalizeValueForKind(target_kind, value, &normalized, out_error)) {
        return false;
    }

    environment_[statement.name] = runtime::VariableSlot{normalized, target_kind};
    return true;
}

bool Interpreter::ApplyVariableMutation(
    const std::string& name,
    frontend::AssignmentOp op,
    const runtime::Value& value,
    std::string* out_error) {
    const auto found = environment_.find(name);
    if (found == environment_.end() && op != frontend::AssignmentOp::Set) {
        *out_error = "Variable no definida: " + name;
        return false;
    }

    runtime::VariableKind target_kind = runtime::VariableKind::Dynamic;
    if (found != environment_.end()) {
        target_kind = found->second.kind;
    }

    runtime::Value value_to_store = value;
    if (op == frontend::AssignmentOp::AddAssign || op == frontend::AssignmentOp::SubAssign) {
        runtime::Value merged;
        if (!EvaluateBinary(
                op == frontend::AssignmentOp::AddAssign ? frontend::BinaryOp::Add : frontend::BinaryOp::Subtract,
                found->second.value,
                value,
                &merged,
                out_error)) {
            return false;
        }
        value_to_store = std::move(merged);
    }

    runtime::Value normalized;
    if (!NormalizeValueForKind(target_kind, value_to_store, &normalized, out_error)) {
        return false;
    }

    if (found == environment_.end()) {
        environment_[name] = runtime::VariableSlot{normalized, runtime::VariableKind::Dynamic};
    } else {
        environment_[name].value = normalized;
    }
    return true;
}

bool Interpreter::ApplyTargetMutation(
    const frontend::Expr& target,
    frontend::AssignmentOp op,
    const runtime::Value& value,
    std::string* out_error) {
    runtime::Value* target_value = nullptr;
    if (!ResolveMutableTarget(target, op == frontend::AssignmentOp::Set, &target_value, out_error)) {
        return false;
    }

    runtime::Value value_to_store = value;
    if (op == frontend::AssignmentOp::AddAssign || op == frontend::AssignmentOp::SubAssign) {
        runtime::Value merged;
        if (!EvaluateBinary(
                op == frontend::AssignmentOp::AddAssign ? frontend::BinaryOp::Add : frontend::BinaryOp::Subtract,
                *target_value,
                value,
                &merged,
                out_error)) {
            return false;
        }
        value_to_store = std::move(merged);
    }

    *target_value = std::move(value_to_store);
    return true;
}

}  // namespace clot::interpreter
