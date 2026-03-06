#include "clot/interpreter/interpreter.hpp"

#include <iomanip>
#include <cmath>
#include <limits>
#include <sstream>
#include <string>
#include <utility>

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

bool TruncateDoubleToBigInt(double value, BigInt* out_integer) {
    if (out_integer == nullptr || !std::isfinite(value)) {
        return false;
    }

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(0) << std::trunc(value);
    return runtime::Value::TryParseBigInt(stream.str(), out_integer);
}

bool ReadListIndex(const runtime::Value& value, std::size_t* out_index, std::string* out_error) {
    BigInt integer_index;
    if (!value.AsBigInt(&integer_index)) {
        if (out_error != nullptr) {
            *out_error = "El indice de lista debe ser un entero finito.";
        }
        return false;
    }

    if (integer_index < 0) {
        if (out_error != nullptr) {
            *out_error = "El indice de lista debe ser un entero finito.";
        }
        return false;
    }

    static const BigInt kMaxIndex = [] {
        BigInt parsed;
        (void)BigInt::TryParse(std::to_string(std::numeric_limits<std::size_t>::max()), &parsed);
        return parsed;
    }();
    if (integer_index > kMaxIndex) {
        if (out_error != nullptr) {
            *out_error = "El indice de lista debe ser un entero finito.";
        }
        return false;
    }

    *out_index = integer_index.convert_to<std::size_t>();
    return true;
}

const char* TypeHintName(frontend::TypeHint hint) {
    switch (hint) {
    case frontend::TypeHint::Inferred:
        return "dynamic";
    case frontend::TypeHint::Int:
        return "int";
    case frontend::TypeHint::Double:
        return "double";
    case frontend::TypeHint::Float:
        return "float";
    case frontend::TypeHint::Decimal:
        return "decimal";
    case frontend::TypeHint::Long:
        return "long";
    case frontend::TypeHint::Byte:
        return "byte";
    case frontend::TypeHint::Char:
        return "char";
    case frontend::TypeHint::Tuple:
        return "tuple";
    case frontend::TypeHint::Set:
        return "set";
    case frontend::TypeHint::Map:
        return "map";
    case frontend::TypeHint::Function:
        return "function";
    case frontend::TypeHint::String:
        return "string";
    case frontend::TypeHint::Bool:
        return "bool";
    case frontend::TypeHint::List:
        return "list";
    case frontend::TypeHint::Object:
        return "object";
    case frontend::TypeHint::Null:
        return "null";
    }
    return "dynamic";
}

bool TryMapTypeHintToVariableKind(frontend::TypeHint hint, runtime::VariableKind* out_kind) {
    if (out_kind == nullptr) {
        return false;
    }

    switch (hint) {
    case frontend::TypeHint::Int:
        *out_kind = runtime::VariableKind::Int;
        return true;
    case frontend::TypeHint::Double:
        *out_kind = runtime::VariableKind::Double;
        return true;
    case frontend::TypeHint::Float:
        *out_kind = runtime::VariableKind::Float;
        return true;
    case frontend::TypeHint::Decimal:
        *out_kind = runtime::VariableKind::Decimal;
        return true;
    case frontend::TypeHint::Long:
        *out_kind = runtime::VariableKind::Long;
        return true;
    case frontend::TypeHint::Byte:
        *out_kind = runtime::VariableKind::Byte;
        return true;
    case frontend::TypeHint::Char:
        *out_kind = runtime::VariableKind::Char;
        return true;
    case frontend::TypeHint::Tuple:
        *out_kind = runtime::VariableKind::Tuple;
        return true;
    case frontend::TypeHint::Set:
        *out_kind = runtime::VariableKind::Set;
        return true;
    case frontend::TypeHint::Map:
        *out_kind = runtime::VariableKind::Map;
        return true;
    case frontend::TypeHint::Function:
        *out_kind = runtime::VariableKind::Function;
        return true;
    case frontend::TypeHint::Inferred:
    case frontend::TypeHint::String:
    case frontend::TypeHint::Bool:
    case frontend::TypeHint::List:
    case frontend::TypeHint::Object:
    case frontend::TypeHint::Null:
        return false;
    }
    return false;
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
            if (name == "endl") {
                *out_value = runtime::Value("\n");
                return true;
            }
            const auto function = functions_.find(name);
            if (function != functions_.end()) {
                *out_value = runtime::Value(runtime::Value::FunctionRef{name});
                return true;
            }
            if (imported_modules_.count("math") > 0) {
                if (name == "PI" || name == "pi") {
                    *out_value = runtime::Value(3.14159265358979323846);
                    return true;
                }
                if (name == "E" || name == "e") {
                    *out_value = runtime::Value(2.71828182845904523536);
                    return true;
                }
                if (name == "TAU" || name == "tau") {
                    *out_value = runtime::Value(6.28318530717958647693);
                    return true;
                }
                if (name == "PHI" || name == "phi") {
                    *out_value = runtime::Value(1.61803398874989484820);
                    return true;
                }
            }
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

        if (runtime::Value::List* list = collection->MutableList()) {
            std::size_t integer_index = 0;
            if (!ReadListIndex(index_value, &integer_index, out_error)) {
                return false;
            }

            if (integer_index >= list->size()) {
                *out_error = "Indice fuera de rango en lista.";
                return false;
            }

            *out_value = &(*list)[integer_index];
            return true;
        }

        if (collection->AsTuple() != nullptr) {
            *out_error = "No se puede mutar un tuple con [].";
            return false;
        }

        if (collection->IsMap()) {
            if (create_missing_property) {
                *out_value = collection->EnsureMapValue(index_value);
            } else {
                *out_value = collection->GetMutableMapValue(index_value);
            }
            if (*out_value == nullptr) {
                *out_error = "Clave no encontrada en map.";
                return false;
            }
            return true;
        }

        if (collection->IsObject()) {
            const std::string key = index_value.ToString();
            if (create_missing_property) {
                *out_value = collection->EnsureObjectProperty(key);
            } else {
                *out_value = collection->GetMutableObjectProperty(key);
            }
            if (*out_value == nullptr) {
                *out_error = "Propiedad no encontrada: " + key;
                return false;
            }
            return true;
        }

        *out_error = "Solo se puede mutar list, map u object con [].";
        return false;
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
    if (kind == runtime::VariableKind::Dynamic) {
        *out_value = std::move(normalized);
        return true;
    }

    if (kind == runtime::VariableKind::Double) {
        double numeric = 0.0;
        if (!ReadNumeric(value, &numeric, out_error)) {
            return false;
        }
        if (!std::isfinite(numeric)) {
            *out_error = "Valor no finito para double.";
            return false;
        }
        normalized = runtime::Value(numeric);
        *out_value = std::move(normalized);
        return true;
    }

    if (kind == runtime::VariableKind::Float) {
        double numeric = 0.0;
        if (!ReadNumeric(value, &numeric, out_error)) {
            return false;
        }
        if (!std::isfinite(numeric) ||
            numeric < -static_cast<double>(std::numeric_limits<float>::max()) ||
            numeric > static_cast<double>(std::numeric_limits<float>::max())) {
            *out_error = "Valor fuera de rango para float.";
            return false;
        }
        normalized = runtime::Value(static_cast<float>(numeric));
        *out_value = std::move(normalized);
        return true;
    }

    if (kind == runtime::VariableKind::Decimal) {
        runtime::Value::Decimal decimal;
        if (!value.AsDecimal(&decimal)) {
            *out_error = "La expresion requiere un decimal.";
            return false;
        }
        normalized = runtime::Value(std::move(decimal));
        *out_value = std::move(normalized);
        return true;
    }

    if (kind == runtime::VariableKind::Char) {
        if (value.IsChar()) {
            *out_value = value;
            return true;
        }

        if (value.IsString()) {
            const std::string text = value.ToString();
            if (text.size() != 1) {
                *out_error = "Valor invalido para char (requiere longitud 1).";
                return false;
            }
            *out_value = runtime::Value(text[0]);
            return true;
        }

        BigInt integer;
        if (value.AsBigInt(&integer) && integer >= 0 && integer <= 255) {
            *out_value = runtime::Value(static_cast<char>(static_cast<unsigned char>(integer.convert_to<unsigned>())));
            return true;
        }

        *out_error = "Valor invalido para char.";
        return false;
    }

    if (kind == runtime::VariableKind::Tuple) {
        if (const auto* tuple = value.AsTuple()) {
            *out_value = runtime::Value(runtime::Value::Tuple{*tuple});
            return true;
        }
        if (const auto* list = value.AsList()) {
            *out_value = runtime::Value(runtime::Value::Tuple{*list});
            return true;
        }
        if (const auto* set = value.AsSet()) {
            *out_value = runtime::Value(runtime::Value::Tuple{*set});
            return true;
        }

        *out_error = "Valor invalido para tuple.";
        return false;
    }

    if (kind == runtime::VariableKind::Set) {
        runtime::Value::Set result;
        auto insert_unique = [&result](const runtime::Value& candidate) {
            for (const auto& existing : result.elements) {
                if (existing.Equals(candidate)) {
                    return;
                }
            }
            result.elements.push_back(candidate);
        };

        if (const auto* set = value.AsSet()) {
            for (const auto& element : *set) {
                insert_unique(element);
            }
            *out_value = runtime::Value(std::move(result));
            return true;
        }
        if (const auto* list = value.AsList()) {
            for (const auto& element : *list) {
                insert_unique(element);
            }
            *out_value = runtime::Value(std::move(result));
            return true;
        }
        if (const auto* tuple = value.AsTuple()) {
            for (const auto& element : *tuple) {
                insert_unique(element);
            }
            *out_value = runtime::Value(std::move(result));
            return true;
        }

        insert_unique(value);
        *out_value = runtime::Value(std::move(result));
        return true;
    }

    if (kind == runtime::VariableKind::Map) {
        if (const auto* map = value.AsMap()) {
            *out_value = runtime::Value(runtime::Value::Map{*map});
            return true;
        }

        if (const auto* object = value.AsObject()) {
            runtime::Value::Map map;
            map.entries.reserve(object->size());
            for (const auto& entry : *object) {
                map.entries.push_back({runtime::Value(entry.first), entry.second});
            }
            *out_value = runtime::Value(std::move(map));
            return true;
        }

        *out_error = "Valor invalido para map.";
        return false;
    }

    if (kind == runtime::VariableKind::Function) {
        if (value.IsFunctionRef()) {
            *out_value = value;
            return true;
        }

        if (value.IsString()) {
            const std::string function_name = value.ToString();
            if (functions_.find(function_name) != functions_.end()) {
                *out_value = runtime::Value(runtime::Value::FunctionRef{function_name});
                return true;
            }
        }

        *out_error = "Valor invalido para function.";
        return false;
    }

    BigInt integer;
    if (!value.AsBigInt(&integer)) {
        double numeric = 0.0;
        if (!ReadNumeric(value, &numeric, out_error)) {
            return false;
        }
        if (!TruncateDoubleToBigInt(numeric, &integer)) {
            *out_error = "No se pudo convertir el valor numerico a entero.";
            return false;
        }
    }

    if (kind == runtime::VariableKind::Int) {
        normalized = runtime::Value(std::move(integer));
        *out_value = std::move(normalized);
        return true;
    }

    if (kind == runtime::VariableKind::Long) {
        long long integer64 = 0;
        if (!runtime::Value::TryBigIntToInt64(integer, &integer64)) {
            *out_error = "Valor fuera de rango para long.";
            return false;
        }
        normalized = runtime::Value(integer64);
        *out_value = std::move(normalized);
        return true;
    }

    if (integer < 0 || integer > 255) {
        *out_error = "Valor fuera de rango para byte (0-255).";
        return false;
    }
    normalized = runtime::Value(std::move(integer));

    *out_value = std::move(normalized);
    return true;
}

bool Interpreter::NormalizeValueForTypeHint(
    frontend::TypeHint hint,
    const runtime::Value& value,
    runtime::Value* out_value,
    std::string* out_error) const {
    if (out_value == nullptr) {
        if (out_error != nullptr) {
            *out_error = "Error interno: out_value nulo en NormalizeValueForTypeHint.";
        }
        return false;
    }

    if (hint == frontend::TypeHint::Inferred) {
        *out_value = value;
        return true;
    }

    runtime::VariableKind kind = runtime::VariableKind::Dynamic;
    if (TryMapTypeHintToVariableKind(hint, &kind)) {
        return NormalizeValueForKind(kind, value, out_value, out_error);
    }

    if (hint == frontend::TypeHint::String) {
        if (!value.IsString()) {
            *out_error = "La expresion requiere un string.";
            return false;
        }
        *out_value = value;
        return true;
    }

    if (hint == frontend::TypeHint::Bool) {
        if (!value.IsBool()) {
            *out_error = "La expresion requiere un bool.";
            return false;
        }
        *out_value = value;
        return true;
    }

    if (hint == frontend::TypeHint::List) {
        if (!value.IsList()) {
            *out_error = "La expresion requiere un list.";
            return false;
        }
        *out_value = value;
        return true;
    }

    if (hint == frontend::TypeHint::Object) {
        if (!value.IsObject()) {
            *out_error = "La expresion requiere un object.";
            return false;
        }
        *out_value = value;
        return true;
    }

    if (hint == frontend::TypeHint::Null) {
        if (!value.IsNull()) {
            *out_error = "La expresion requiere null.";
            return false;
        }
        *out_value = value;
        return true;
    }

    if (out_error != nullptr) {
        *out_error = std::string("Type hint no soportado: ") + TypeHintName(hint) + ".";
    }
    return false;
}

bool Interpreter::AssignValue(
    const frontend::AssignmentStmt& statement,
    const runtime::Value& value,
    std::string* out_error) {
    if (statement.name.find('.') != std::string::npos) {
        if (statement.declaration_type != frontend::DeclarationType::Inferred) {
            *out_error = "No se puede declarar tipo explicito sobre una propiedad de objeto.";
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

    if (statement.declaration_type == frontend::DeclarationType::Int) {
        target_kind = runtime::VariableKind::Int;
    } else if (statement.declaration_type == frontend::DeclarationType::Double) {
        target_kind = runtime::VariableKind::Double;
    } else if (statement.declaration_type == frontend::DeclarationType::Float) {
        target_kind = runtime::VariableKind::Float;
    } else if (statement.declaration_type == frontend::DeclarationType::Decimal) {
        target_kind = runtime::VariableKind::Decimal;
    } else if (statement.declaration_type == frontend::DeclarationType::Long) {
        target_kind = runtime::VariableKind::Long;
    } else if (statement.declaration_type == frontend::DeclarationType::Byte) {
        target_kind = runtime::VariableKind::Byte;
    } else if (statement.declaration_type == frontend::DeclarationType::Char) {
        target_kind = runtime::VariableKind::Char;
    } else if (statement.declaration_type == frontend::DeclarationType::Tuple) {
        target_kind = runtime::VariableKind::Tuple;
    } else if (statement.declaration_type == frontend::DeclarationType::Set) {
        target_kind = runtime::VariableKind::Set;
    } else if (statement.declaration_type == frontend::DeclarationType::Map) {
        target_kind = runtime::VariableKind::Map;
    } else if (statement.declaration_type == frontend::DeclarationType::Function) {
        target_kind = runtime::VariableKind::Function;
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
