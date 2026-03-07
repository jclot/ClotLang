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

bool ReadInteger(const runtime::Value& value, runtime::Value::BigInt* out_integer, std::string* out_error) {
    if (!value.AsBigInt(out_integer)) {
        if (out_error != nullptr) {
            *out_error = "La expresion requiere un entero.";
        }
        return false;
    }
    return true;
}

bool ReadDecimal(const runtime::Value& value, runtime::Value::Decimal* out_decimal, std::string* out_error) {
    if (!value.AsDecimal(out_decimal)) {
        if (out_error != nullptr) {
            *out_error = "La expresion requiere un decimal.";
        }
        return false;
    }
    return true;
}

bool ReadInteger64(const runtime::Value& value, long long* out_integer) {
    bool ok = false;
    const long long integer = value.AsInteger(&ok);
    if (!ok) {
        return false;
    }

    *out_integer = integer;
    return true;
}

std::string BigIntToHexString(BigInt value, bool uppercase) {
    if (value == 0) {
        return "0";
    }

    static constexpr char kHexLower[] = "0123456789abcdef";
    static constexpr char kHexUpper[] = "0123456789ABCDEF";
    const char* alphabet = uppercase ? kHexUpper : kHexLower;

    std::string text;
    while (value > 0) {
        const BigInt quotient = value / 16;
        const BigInt remainder = value % 16;
        const unsigned index = remainder.convert_to<unsigned>();
        text.push_back(alphabet[index]);
        value = quotient;
    }
    std::reverse(text.begin(), text.end());
    return text;
}

BigInt AbsBigInt(const BigInt& value) {
    return value < 0 ? -value : value;
}

BigInt GcdBigInt(BigInt lhs, BigInt rhs) {
    lhs = AbsBigInt(lhs);
    rhs = AbsBigInt(rhs);
    while (rhs != 0) {
        BigInt next = lhs % rhs;
        lhs = std::move(rhs);
        rhs = std::move(next);
    }
    return lhs;
}

bool ContainsValue(const std::vector<runtime::Value>& values, const runtime::Value& candidate) {
    for (const auto& value : values) {
        if (value.Equals(candidate)) {
            return true;
        }
    }
    return false;
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

bool ReadTaskId(const runtime::Value& value, long long* out_task_id, std::string* out_error) {
    long long task_id = 0;
    if (!ReadInteger64(value, &task_id) || task_id <= 0) {
        if (out_error != nullptr) {
            *out_error = "El id de tarea debe ser un entero positivo.";
        }
        return false;
    }

    *out_task_id = task_id;
    return true;
}

bool ReadFileToString(const std::string& path, std::string* out_text, std::string* out_error) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        if (out_error != nullptr) {
            *out_error = "No se pudo abrir el archivo: " + path;
        }
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        if (out_error != nullptr) {
            *out_error = "Error leyendo el archivo: " + path;
        }
        return false;
    }

    *out_text = buffer.str();
    return true;
}

bool WriteStringToFile(const std::string& path, const std::string& text, bool append, std::string* out_error) {
    std::ofstream output;
    if (append) {
        output.open(path, std::ios::binary | std::ios::app);
    } else {
        output.open(path, std::ios::binary | std::ios::trunc);
    }

    if (!output.is_open()) {
        if (out_error != nullptr) {
            *out_error = "No se pudo abrir el archivo: " + path;
        }
        return false;
    }

    output << text;
    if (!output.good()) {
        if (out_error != nullptr) {
            *out_error = "Error escribiendo el archivo: " + path;
        }
        return false;
    }

    return true;
}

bool RenderPrintfFormat(const std::string& format,
                        const std::vector<runtime::Value>& arguments,
                        std::string* out_text,
                        std::string* out_error) {
    if (out_text == nullptr) {
        if (out_error != nullptr) {
            *out_error = "Error interno: salida nula en printf.";
        }
        return false;
    }

    out_text->clear();
    std::size_t argument_index = 0;

    for (std::size_t i = 0; i < format.size(); ++i) {
        const char current = format[i];
        if (current != '%') {
            out_text->push_back(current);
            continue;
        }

        if (i + 1 >= format.size()) {
            if (out_error != nullptr) {
                *out_error = "printf: formato invalido, '%' sin especificador.";
            }
            return false;
        }

        const char specifier = format[++i];
        if (specifier == '%') {
            out_text->push_back('%');
            continue;
        }

        if (argument_index >= arguments.size()) {
            if (out_error != nullptr) {
                *out_error = "printf: faltan argumentos para el formato.";
            }
            return false;
        }

        const runtime::Value& argument = arguments[argument_index++];
        switch (specifier) {
        case 'd':
        case 'i': {
            bool ok = false;
            const long long integer = argument.AsInteger(&ok);
            if (!ok) {
                if (out_error != nullptr) {
                    *out_error = "printf: %d/%i requiere entero.";
                }
                return false;
            }
            *out_text += std::to_string(integer);
            break;
        }
        case 'u': {
            bool ok = false;
            const long long integer = argument.AsInteger(&ok);
            if (!ok || integer < 0) {
                if (out_error != nullptr) {
                    *out_error = "printf: %u requiere entero sin signo (>= 0).";
                }
                return false;
            }
            *out_text += std::to_string(static_cast<unsigned long long>(integer));
            break;
        }
        case 'f': {
            bool ok = false;
            const double number = argument.AsNumber(&ok);
            if (!ok) {
                if (out_error != nullptr) {
                    *out_error = "printf: %f requiere valor numerico.";
                }
                return false;
            }
            std::ostringstream stream;
            stream << std::fixed << std::setprecision(6) << number;
            *out_text += stream.str();
            break;
        }
        case 'c': {
            bool emitted = false;
            if (argument.IsString()) {
                const std::string text = argument.ToString();
                if (text.size() == 1) {
                    out_text->push_back(text[0]);
                    emitted = true;
                }
            }

            if (!emitted) {
                bool ok = false;
                const long long integer = argument.AsInteger(&ok);
                if (ok && integer >= 0 && integer <= 255) {
                    out_text->push_back(static_cast<char>(static_cast<unsigned char>(integer)));
                    emitted = true;
                }
            }

            if (!emitted) {
                if (out_error != nullptr) {
                    *out_error = "printf: %c requiere char (string de longitud 1 o entero ASCII 0-255).";
                }
                return false;
            }
            break;
        }
        case 's':
            *out_text += argument.ToString();
            break;
        case 'x':
        case 'X': {
            BigInt integer;
            if (!argument.AsBigInt(&integer) || integer < 0) {
                if (out_error != nullptr) {
                    *out_error = "printf: %x/%X requiere entero sin signo (>= 0).";
                }
                return false;
            }

            *out_text += BigIntToHexString(integer, specifier == 'X');
            break;
        }
        default:
            if (out_error != nullptr) {
                *out_error = std::string("printf: especificador no soportado '%") + specifier + "'.";
            }
            return false;
        }
    }

    if (argument_index != arguments.size()) {
        if (out_error != nullptr) {
            *out_error = "printf: sobran argumentos para el formato.";
        }
        return false;
    }

    return true;
}

} // namespace

bool Interpreter::ExecuteBuiltinCall(const frontend::CallExpr& call, bool* out_was_builtin, runtime::Value* out_value,
                                     std::string* out_error) {
    *out_was_builtin = false;
    const bool math_imported = imported_modules_.count("math") > 0;

    auto evaluate_argument = [&](std::size_t index, runtime::Value* out_argument) -> bool {
        if (index >= call.arguments.size()) {
            if (out_error != nullptr) {
                *out_error = "Error interno: indice de argumento invalido.";
            }
            return false;
        }
        if (call.arguments[index].value == nullptr) {
            if (out_error != nullptr) {
                *out_error = "Error interno: argumento de llamada vacio.";
            }
            return false;
        }
        return EvaluateExpression(*call.arguments[index].value, out_argument, out_error);
    };

    if (call.callee == "sum" && math_imported) {
        *out_was_builtin = true;

        if (call.arguments.size() != 2) {
            *out_error = "sum(a, b) requiere 2 argumentos.";
            return false;
        }

        runtime::Value left;
        runtime::Value right;
        if (!evaluate_argument(0, &left) || !evaluate_argument(1, &right)) {
            return false;
        }

        if (left.IsDecimal() || right.IsDecimal()) {
            runtime::Value::Decimal left_decimal;
            runtime::Value::Decimal right_decimal;
            if (!ReadDecimal(left, &left_decimal, out_error) || !ReadDecimal(right, &right_decimal, out_error)) {
                return false;
            }
            *out_value = runtime::Value(left_decimal + right_decimal);
            return true;
        }

        BigInt left_integer;
        BigInt right_integer;
        if (left.AsBigInt(&left_integer) && right.AsBigInt(&right_integer)) {
            *out_value = runtime::Value(left_integer + right_integer);
            return true;
        }

        double left_number = 0.0;
        double right_number = 0.0;
        if (!ReadNumeric(left, &left_number, out_error) || !ReadNumeric(right, &right_number, out_error)) {
            return false;
        }
        *out_value = runtime::Value(left_number + right_number);
        return true;
    }

    if (call.callee == "factorial" && math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "factorial() requiere 1 argumento.";
            return false;
        }

        runtime::Value value;
        if (!evaluate_argument(0, &value)) {
            return false;
        }

        BigInt n;
        if (!ReadInteger(value, &n, out_error)) {
            return false;
        }
        if (n < 0) {
            *out_error = "factorial() requiere un entero no negativo.";
            return false;
        }
        if (n > 100000) {
            *out_error = "factorial() argumento demasiado grande.";
            return false;
        }

        const unsigned long long limit = n.convert_to<unsigned long long>();
        BigInt result = 1;
        for (unsigned long long i = 2; i <= limit; ++i) {
            result *= i;
        }

        *out_value = runtime::Value(std::move(result));
        return true;
    }

    if (call.callee == "sqrt" && math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "sqrt(x) requiere 1 argumento.";
            return false;
        }

        runtime::Value value;
        if (!evaluate_argument(0, &value)) {
            return false;
        }
        double numeric = 0.0;
        if (!ReadNumeric(value, &numeric, out_error)) {
            return false;
        }
        if (numeric < 0.0) {
            *out_error = "sqrt(x) requiere x >= 0.";
            return false;
        }
        *out_value = runtime::Value(std::sqrt(numeric));
        return true;
    }

    if (call.callee == "pow" && math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 2) {
            *out_error = "pow(a, b) requiere 2 argumentos.";
            return false;
        }

        runtime::Value base_value;
        runtime::Value exponent_value;
        if (!evaluate_argument(0, &base_value) || !evaluate_argument(1, &exponent_value)) {
            return false;
        }

        BigInt base_integer;
        BigInt exponent_integer;
        if (base_value.AsBigInt(&base_integer) && exponent_value.AsBigInt(&exponent_integer) && exponent_integer >= 0) {
            BigInt result;
            if (!PowBigInt(base_integer, exponent_integer, &result, out_error)) {
                return false;
            }
            *out_value = runtime::Value(std::move(result));
            return true;
        }

        double base = 0.0;
        double exponent = 0.0;
        if (!ReadNumeric(base_value, &base, out_error) || !ReadNumeric(exponent_value, &exponent, out_error)) {
            return false;
        }
        *out_value = runtime::Value(std::pow(base, exponent));
        return true;
    }

    if (call.callee == "log" && math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 1 && call.arguments.size() != 2) {
            *out_error = "log(x) o log(x, base) requiere 1 o 2 argumentos.";
            return false;
        }

        runtime::Value x_value;
        if (!evaluate_argument(0, &x_value)) {
            return false;
        }
        double x = 0.0;
        if (!ReadNumeric(x_value, &x, out_error)) {
            return false;
        }
        if (x <= 0.0) {
            *out_error = "log(x) requiere x > 0.";
            return false;
        }

        if (call.arguments.size() == 1) {
            *out_value = runtime::Value(std::log10(x));
            return true;
        }

        runtime::Value base_value;
        if (!evaluate_argument(1, &base_value)) {
            return false;
        }
        double base = 0.0;
        if (!ReadNumeric(base_value, &base, out_error)) {
            return false;
        }
        if (base <= 0.0 || base == 1.0) {
            *out_error = "log(x, base) requiere base > 0 y base != 1.";
            return false;
        }

        *out_value = runtime::Value(std::log(x) / std::log(base));
        return true;
    }

    if (call.callee == "ln" && math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "ln(x) requiere 1 argumento.";
            return false;
        }

        runtime::Value value;
        if (!evaluate_argument(0, &value)) {
            return false;
        }
        double numeric = 0.0;
        if (!ReadNumeric(value, &numeric, out_error)) {
            return false;
        }
        if (numeric <= 0.0) {
            *out_error = "ln(x) requiere x > 0.";
            return false;
        }
        *out_value = runtime::Value(std::log(numeric));
        return true;
    }

    if (call.callee == "exp" && math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "exp(x) requiere 1 argumento.";
            return false;
        }

        runtime::Value value;
        if (!evaluate_argument(0, &value)) {
            return false;
        }
        double numeric = 0.0;
        if (!ReadNumeric(value, &numeric, out_error)) {
            return false;
        }
        *out_value = runtime::Value(std::exp(numeric));
        return true;
    }

    if (call.callee == "abs" && math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "abs(x) requiere 1 argumento.";
            return false;
        }

        runtime::Value value;
        if (!evaluate_argument(0, &value)) {
            return false;
        }
        BigInt integer;
        if (value.AsBigInt(&integer)) {
            *out_value = runtime::Value(AbsBigInt(integer));
            return true;
        }
        double numeric = 0.0;
        if (!ReadNumeric(value, &numeric, out_error)) {
            return false;
        }
        *out_value = runtime::Value(std::fabs(numeric));
        return true;
    }

    if ((call.callee == "sin" || call.callee == "cos" || call.callee == "tan" || call.callee == "asin" ||
         call.callee == "acos" || call.callee == "atan") &&
        math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = call.callee + "(x) requiere 1 argumento.";
            return false;
        }

        runtime::Value value;
        if (!evaluate_argument(0, &value)) {
            return false;
        }
        double numeric = 0.0;
        if (!ReadNumeric(value, &numeric, out_error)) {
            return false;
        }

        if ((call.callee == "asin" || call.callee == "acos") && (numeric < -1.0 || numeric > 1.0)) {
            *out_error = call.callee + "(x) requiere -1 <= x <= 1.";
            return false;
        }

        if (call.callee == "sin") {
            *out_value = runtime::Value(std::sin(numeric));
        } else if (call.callee == "cos") {
            *out_value = runtime::Value(std::cos(numeric));
        } else if (call.callee == "tan") {
            *out_value = runtime::Value(std::tan(numeric));
        } else if (call.callee == "asin") {
            *out_value = runtime::Value(std::asin(numeric));
        } else if (call.callee == "acos") {
            *out_value = runtime::Value(std::acos(numeric));
        } else {
            *out_value = runtime::Value(std::atan(numeric));
        }
        return true;
    }

    if (call.callee == "gcd" && math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 2) {
            *out_error = "gcd(a, b) requiere 2 argumentos.";
            return false;
        }

        runtime::Value left_value;
        runtime::Value right_value;
        if (!evaluate_argument(0, &left_value) || !evaluate_argument(1, &right_value)) {
            return false;
        }

        BigInt left;
        BigInt right;
        if (!ReadInteger(left_value, &left, out_error) || !ReadInteger(right_value, &right, out_error)) {
            return false;
        }

        *out_value = runtime::Value(GcdBigInt(left, right));
        return true;
    }

    if (call.callee == "lcm" && math_imported) {
        *out_was_builtin = true;
        if (call.arguments.size() != 2) {
            *out_error = "lcm(a, b) requiere 2 argumentos.";
            return false;
        }

        runtime::Value left_value;
        runtime::Value right_value;
        if (!evaluate_argument(0, &left_value) || !evaluate_argument(1, &right_value)) {
            return false;
        }

        BigInt left;
        BigInt right;
        if (!ReadInteger(left_value, &left, out_error) || !ReadInteger(right_value, &right, out_error)) {
            return false;
        }

        if (left == 0 || right == 0) {
            *out_value = runtime::Value(BigInt(0));
            return true;
        }

        const BigInt gcd = GcdBigInt(left, right);
        const BigInt lcm = AbsBigInt((left / gcd) * right);
        *out_value = runtime::Value(lcm);
        return true;
    }

    if (call.callee == "tuple") {
        *out_was_builtin = true;
        runtime::Value::Tuple tuple_value;
        tuple_value.elements.reserve(call.arguments.size());
        for (std::size_t i = 0; i < call.arguments.size(); ++i) {
            runtime::Value element;
            if (!evaluate_argument(i, &element)) {
                return false;
            }
            tuple_value.elements.push_back(std::move(element));
        }
        *out_value = runtime::Value(std::move(tuple_value));
        return true;
    }

    if (call.callee == "set") {
        *out_was_builtin = true;
        runtime::Value::Set set_value;

        auto push_unique = [&](const runtime::Value& candidate) {
            if (!ContainsValue(set_value.elements, candidate)) {
                set_value.elements.push_back(candidate);
            }
        };

        if (call.arguments.size() == 1) {
            runtime::Value source;
            if (!evaluate_argument(0, &source)) {
                return false;
            }
            if (const auto* list = source.AsList()) {
                for (const auto& element : *list) {
                    push_unique(element);
                }
                *out_value = runtime::Value(std::move(set_value));
                return true;
            }
            if (const auto* tuple = source.AsTuple()) {
                for (const auto& element : *tuple) {
                    push_unique(element);
                }
                *out_value = runtime::Value(std::move(set_value));
                return true;
            }
            if (const auto* set = source.AsSet()) {
                for (const auto& element : *set) {
                    push_unique(element);
                }
                *out_value = runtime::Value(std::move(set_value));
                return true;
            }
        }

        for (std::size_t i = 0; i < call.arguments.size(); ++i) {
            runtime::Value element;
            if (!evaluate_argument(i, &element)) {
                return false;
            }
            push_unique(element);
        }
        *out_value = runtime::Value(std::move(set_value));
        return true;
    }

    if (call.callee == "map") {
        *out_was_builtin = true;
        if ((call.arguments.size() % 2) != 0) {
            *out_error = "map(key, value, ...) requiere cantidad par de argumentos.";
            return false;
        }

        runtime::Value::Map map_value;
        for (std::size_t i = 0; i < call.arguments.size(); i += 2) {
            runtime::Value key;
            runtime::Value value;
            if (!evaluate_argument(i, &key) || !evaluate_argument(i + 1, &value)) {
                return false;
            }

            bool replaced = false;
            for (auto& entry : map_value.entries) {
                if (entry.first.Equals(key)) {
                    entry.second = std::move(value);
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                map_value.entries.push_back({std::move(key), std::move(value)});
            }
        }

        *out_value = runtime::Value(std::move(map_value));
        return true;
    }

    if (call.callee == "enum_name") {
        *out_was_builtin = true;
        if (call.arguments.size() != 2) {
            *out_error = "enum_name(enum_obj, value) requiere 2 argumentos.";
            return false;
        }

        runtime::Value enum_object;
        runtime::Value enum_value;
        if (!evaluate_argument(0, &enum_object) || !evaluate_argument(1, &enum_value)) {
            return false;
        }

        if (!enum_object.IsObject()) {
            *out_error = "enum_name(): primer argumento debe ser enum.";
            return false;
        }

        const runtime::Value* lookup = enum_object.GetObjectProperty("__value_to_name");
        if (lookup == nullptr || !lookup->IsMap()) {
            *out_error = "enum_name(): primer argumento debe ser enum.";
            return false;
        }

        const runtime::Value* name = lookup->GetMapValue(enum_value);
        if (name == nullptr) {
            *out_error = "enum_name(): valor no encontrado en enum.";
            return false;
        }

        *out_value = *name;
        return true;
    }

    if (call.callee == "enum_value") {
        *out_was_builtin = true;
        if (call.arguments.size() != 2) {
            *out_error = "enum_value(enum_obj, name) requiere 2 argumentos.";
            return false;
        }

        runtime::Value enum_object;
        runtime::Value member_name;
        if (!evaluate_argument(0, &enum_object) || !evaluate_argument(1, &member_name)) {
            return false;
        }

        if (!enum_object.IsObject()) {
            *out_error = "enum_value(): primer argumento debe ser enum.";
            return false;
        }

        if (!member_name.IsString() && !member_name.IsChar()) {
            *out_error = "enum_value(): nombre invalido.";
            return false;
        }

        const runtime::Value* lookup = enum_object.GetObjectProperty("__name_to_value");
        if (lookup == nullptr || !lookup->IsMap()) {
            *out_error = "enum_value(): primer argumento debe ser enum.";
            return false;
        }

        const runtime::Value* value = lookup->GetMapValue(runtime::Value(member_name.ToString()));
        if (value == nullptr) {
            *out_error = "enum_value(): miembro no encontrado en enum.";
            return false;
        }

        *out_value = *value;
        return true;
    }

    if (call.callee == "type") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "type(value) requiere 1 argumento.";
            return false;
        }

        runtime::Value value;
        if (!evaluate_argument(0, &value)) {
            return false;
        }

        if (value.IsInteger()) {
            *out_value = runtime::Value("int");
        } else if (value.IsDouble()) {
            *out_value = runtime::Value("double");
        } else if (value.IsFloat()) {
            *out_value = runtime::Value("float");
        } else if (value.IsDecimal()) {
            *out_value = runtime::Value("decimal");
        } else if (value.IsChar()) {
            *out_value = runtime::Value("char");
        } else if (value.IsString()) {
            *out_value = runtime::Value("string");
        } else if (value.IsBool()) {
            *out_value = runtime::Value("bool");
        } else if (value.IsNull()) {
            *out_value = runtime::Value("null");
        } else if (value.IsList()) {
            *out_value = runtime::Value("list");
        } else if (value.IsTuple()) {
            *out_value = runtime::Value("tuple");
        } else if (value.IsSet()) {
            *out_value = runtime::Value("set");
        } else if (value.IsMap()) {
            *out_value = runtime::Value("map");
        } else if (value.IsFunctionRef()) {
            *out_value = runtime::Value("function");
        } else {
            *out_value = runtime::Value("object");
        }
        return true;
    }

    if (call.callee == "cast") {
        *out_was_builtin = true;
        if (call.arguments.size() != 2) {
            *out_error = "cast(value, type_name) requiere 2 argumentos.";
            return false;
        }

        runtime::Value source;
        runtime::Value type_name_value;
        if (!evaluate_argument(0, &source) || !evaluate_argument(1, &type_name_value)) {
            return false;
        }

        std::string type_name = type_name_value.ToString();
        std::transform(type_name.begin(), type_name.end(), type_name.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

        if (type_name == "int") {
            return NormalizeValueForKind(runtime::VariableKind::Int, source, out_value, out_error);
        }
        if (type_name == "double") {
            return NormalizeValueForKind(runtime::VariableKind::Double, source, out_value, out_error);
        }
        if (type_name == "float") {
            return NormalizeValueForKind(runtime::VariableKind::Float, source, out_value, out_error);
        }
        if (type_name == "decimal") {
            return NormalizeValueForKind(runtime::VariableKind::Decimal, source, out_value, out_error);
        }
        if (type_name == "long") {
            return NormalizeValueForKind(runtime::VariableKind::Long, source, out_value, out_error);
        }
        if (type_name == "byte") {
            return NormalizeValueForKind(runtime::VariableKind::Byte, source, out_value, out_error);
        }
        if (type_name == "char") {
            return NormalizeValueForKind(runtime::VariableKind::Char, source, out_value, out_error);
        }
        if (type_name == "tuple") {
            return NormalizeValueForKind(runtime::VariableKind::Tuple, source, out_value, out_error);
        }
        if (type_name == "set") {
            return NormalizeValueForKind(runtime::VariableKind::Set, source, out_value, out_error);
        }
        if (type_name == "map") {
            return NormalizeValueForKind(runtime::VariableKind::Map, source, out_value, out_error);
        }
        if (type_name == "function") {
            return NormalizeValueForKind(runtime::VariableKind::Function, source, out_value, out_error);
        }
        if (type_name == "string") {
            *out_value = runtime::Value(source.ToString());
            return true;
        }
        if (type_name == "bool") {
            *out_value = runtime::Value(source.AsBool());
            return true;
        }
        if (type_name == "null") {
            *out_value = runtime::Value(nullptr);
            return true;
        }
        if (type_name == "list") {
            if (source.IsList()) {
                *out_value = source;
                return true;
            }
            if (const auto* tuple = source.AsTuple()) {
                *out_value = runtime::Value(*tuple);
                return true;
            }
            if (const auto* set = source.AsSet()) {
                *out_value = runtime::Value(*set);
                return true;
            }
            *out_error = "cast(): tipo destino no soportado: list";
            return false;
        }
        if (type_name == "object") {
            if (source.IsObject()) {
                *out_value = source;
                return true;
            }
            if (const auto* map = source.AsMap()) {
                runtime::Value::Object object;
                object.reserve(map->size());
                for (const auto& entry : *map) {
                    object.push_back({entry.first.ToString(), entry.second});
                }
                *out_value = runtime::Value(std::move(object));
                return true;
            }
            *out_error = "cast(): tipo destino no soportado: object";
            return false;
        }

        *out_error = "cast(): tipo destino no soportado: " + type_name;
        return false;
    }

    if (call.callee == "assert") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1 && call.arguments.size() != 2) {
            *out_error = "assert(cond) o assert(cond, mensaje) requiere 1 o 2 argumentos.";
            return false;
        }

        runtime::Value condition;
        if (!evaluate_argument(0, &condition)) {
            return false;
        }

        if (condition.AsBool()) {
            *out_value = runtime::Value(true);
            return true;
        }

        if (call.arguments.size() == 2) {
            runtime::Value message;
            if (!evaluate_argument(1, &message)) {
                return false;
            }
            *out_error = "assert() fallo: " + message.ToString();
        } else {
            *out_error = "assert() fallo.";
        }
        return false;
    }

    if (call.callee == "throw") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "throw(value) requiere 1 argumento.";
            return false;
        }

        runtime::Value value;
        if (!evaluate_argument(0, &value)) {
            return false;
        }

        return RaiseExceptionValue(value, out_error);
    }

    if (call.callee == "input") {
        *out_was_builtin = true;

        if (call.arguments.size() > 1) {
            *out_error = "input() acepta 0 o 1 argumento.";
            return false;
        }

        if (call.arguments.size() == 1) {
            runtime::Value prompt;
            if (!evaluate_argument(0, &prompt)) {
                return false;
            }
            std::cout << prompt.ToString() << std::flush;
        }

        std::string line;
        std::getline(std::cin, line);
        *out_value = runtime::Value(line);
        return true;
    }

    if (call.callee == "println") {
        *out_was_builtin = true;

        if (call.arguments.size() > 1) {
            *out_error = "println() acepta 0 o 1 argumento.";
            return false;
        }

        if (call.arguments.size() == 1) {
            runtime::Value value;
            if (!evaluate_argument(0, &value)) {
                return false;
            }
            std::cout << value.ToString();
        }

        std::cout << std::endl;
        *out_value = runtime::Value(0LL);
        return true;
    }

    if (call.callee == "printf") {
        *out_was_builtin = true;

        if (call.arguments.empty()) {
            *out_error = "printf(format, ...args) requiere al menos 1 argumento.";
            return false;
        }

        runtime::Value format_value;
        if (!evaluate_argument(0, &format_value)) {
            return false;
        }

        const std::string format = format_value.ToString();
        std::vector<runtime::Value> format_arguments;
        format_arguments.reserve(call.arguments.size() - 1);

        for (std::size_t i = 1; i < call.arguments.size(); ++i) {
            runtime::Value evaluated;
            if (!evaluate_argument(i, &evaluated)) {
                return false;
            }
            format_arguments.push_back(std::move(evaluated));
        }

        std::string rendered;
        if (!RenderPrintfFormat(format, format_arguments, &rendered, out_error)) {
            return false;
        }

        std::cout << rendered << std::flush;
        *out_value = runtime::Value(static_cast<long long>(rendered.size()));
        return true;
    }

    if (call.callee == "read_file") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "read_file(path) requiere 1 argumento.";
            return false;
        }

        runtime::Value path_value;
        if (!evaluate_argument(0, &path_value)) {
            return false;
        }

        const std::string path = path_value.ToString();
        std::string text;
        if (!ReadFileToString(path, &text, out_error)) {
            return false;
        }

        *out_value = runtime::Value(text);
        return true;
    }

    if (call.callee == "write_file" || call.callee == "append_file") {
        *out_was_builtin = true;
        if (call.arguments.size() != 2) {
            *out_error = call.callee + "(path, content) requiere 2 argumentos.";
            return false;
        }

        runtime::Value path_value;
        runtime::Value content_value;
        if (!evaluate_argument(0, &path_value) || !evaluate_argument(1, &content_value)) {
            return false;
        }

        const std::string path = path_value.ToString();
        const std::string content = content_value.ToString();
        if (!WriteStringToFile(path, content, call.callee == "append_file", out_error)) {
            return false;
        }

        *out_value = runtime::Value(true);
        return true;
    }

    if (call.callee == "file_exists") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "file_exists(path) requiere 1 argumento.";
            return false;
        }

        runtime::Value path_value;
        if (!evaluate_argument(0, &path_value)) {
            return false;
        }

        const std::string path = path_value.ToString();
        *out_value = runtime::Value(std::filesystem::exists(std::filesystem::path(path)));
        return true;
    }

    if (call.callee == "now_ms") {
        *out_was_builtin = true;
        if (!call.arguments.empty()) {
            *out_error = "now_ms() no acepta argumentos.";
            return false;
        }

        const auto now = std::chrono::system_clock::now().time_since_epoch();
        const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
        *out_value = runtime::Value(static_cast<long long>(millis));
        return true;
    }

    if (call.callee == "sleep_ms") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "sleep_ms(ms) requiere 1 argumento.";
            return false;
        }

        runtime::Value delay_value;
        if (!evaluate_argument(0, &delay_value)) {
            return false;
        }

        long long delay_ms = 0;
        if (!ReadInteger64(delay_value, &delay_ms) || delay_ms < 0) {
            *out_error = "sleep_ms(ms) requiere entero >= 0.";
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        *out_value = runtime::Value(0LL);
        return true;
    }

    if (call.callee == "async_read_file") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "async_read_file(path) requiere 1 argumento.";
            return false;
        }

        runtime::Value path_value;
        if (!evaluate_argument(0, &path_value)) {
            return false;
        }

        const std::string path = path_value.ToString();
        const long long task_id = next_async_task_id_++;
        async_tasks_[task_id] = AsyncTaskState{
            std::async(std::launch::async,
                       [path]() -> AsyncTaskResult {
                           AsyncTaskResult result;
                           std::string text;
                           std::string error;
                           if (!ReadFileToString(path, &text, &error)) {
                               result.ok = false;
                               result.error = std::move(error);
                               return result;
                           }

                           result.ok = true;
                           result.value = runtime::Value(std::move(text));
                           return result;
                       }),
        };

        *out_value = runtime::Value(task_id);
        return true;
    }

    if (call.callee == "task_ready") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "task_ready(task_id) requiere 1 argumento.";
            return false;
        }

        runtime::Value task_id_value;
        if (!evaluate_argument(0, &task_id_value)) {
            return false;
        }

        long long task_id = 0;
        if (!ReadTaskId(task_id_value, &task_id, out_error)) {
            return false;
        }

        const auto task_it = async_tasks_.find(task_id);
        if (task_it == async_tasks_.end()) {
            *out_error = "Id de tarea no encontrado: " + std::to_string(task_id);
            return false;
        }

        const auto status = task_it->second.future.wait_for(std::chrono::milliseconds(0));
        *out_value = runtime::Value(status == std::future_status::ready);
        return true;
    }

    if (call.callee == "await") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "await(task_id) requiere 1 argumento.";
            return false;
        }

        runtime::Value task_id_value;
        if (!evaluate_argument(0, &task_id_value)) {
            return false;
        }

        long long task_id = 0;
        if (!ReadTaskId(task_id_value, &task_id, out_error)) {
            return false;
        }

        const auto task_it = async_tasks_.find(task_id);
        if (task_it == async_tasks_.end()) {
            *out_error = "Id de tarea no encontrado: " + std::to_string(task_id);
            return false;
        }

        AsyncTaskResult result = task_it->second.future.get();
        async_tasks_.erase(task_it);

        if (!result.ok) {
            *out_error = result.error;
            return false;
        }

        *out_value = result.value;
        return true;
    }

    return true;
}


} // namespace clot::interpreter
