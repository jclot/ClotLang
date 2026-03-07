#include "clot/interpreter/interpreter.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string_view>
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

std::string BigIntToBinaryString(BigInt value) {
    if (value == 0) {
        return "0";
    }

    std::string text;
    while (value > 0) {
        const BigInt quotient = value / 2;
        const BigInt remainder = value % 2;
        text.push_back(remainder == 0 ? '0' : '1');
        value = quotient;
    }
    std::reverse(text.begin(), text.end());
    return text;
}

BigInt SizeToBigInt(std::size_t size) {
    BigInt integer;
    if (!runtime::Value::TryParseBigInt(std::to_string(size), &integer)) {
        return BigInt(0);
    }
    return integer;
}

BigInt UnsignedToBigInt(std::uint64_t value) {
    BigInt integer;
    if (!runtime::Value::TryParseBigInt(std::to_string(value), &integer)) {
        return BigInt(0);
    }
    return integer;
}

std::string LowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string ValueTypeName(const runtime::Value& value) {
    if (value.IsInteger()) {
        return "int";
    }
    if (value.IsDouble()) {
        return "double";
    }
    if (value.IsFloat()) {
        return "float";
    }
    if (value.IsDecimal()) {
        return "decimal";
    }
    if (value.IsChar()) {
        return "char";
    }
    if (value.IsString()) {
        return "string";
    }
    if (value.IsBool()) {
        return "bool";
    }
    if (value.IsNull()) {
        return "null";
    }
    if (value.IsList()) {
        return "list";
    }
    if (value.IsTuple()) {
        return "tuple";
    }
    if (value.IsSet()) {
        return "set";
    }
    if (value.IsMap()) {
        return "map";
    }
    if (value.IsFunctionRef()) {
        return "function";
    }
    return "object";
}

bool MatchesBuiltinTypeName(const runtime::Value& value, const std::string& lowered_type_name) {
    if (lowered_type_name == "any" || lowered_type_name == "dynamic") {
        return true;
    }
    if (lowered_type_name == "number" || lowered_type_name == "numeric") {
        return value.IsNumber();
    }
    if (lowered_type_name == "int" || lowered_type_name == "long" || lowered_type_name == "byte") {
        return value.IsInteger();
    }
    if (lowered_type_name == "double") {
        return value.IsDouble();
    }
    if (lowered_type_name == "float") {
        return value.IsFloat();
    }
    if (lowered_type_name == "decimal") {
        return value.IsDecimal();
    }
    if (lowered_type_name == "char") {
        return value.IsChar();
    }
    if (lowered_type_name == "string") {
        return value.IsString();
    }
    if (lowered_type_name == "bool" || lowered_type_name == "boolean") {
        return value.IsBool();
    }
    if (lowered_type_name == "null") {
        return value.IsNull();
    }
    if (lowered_type_name == "list") {
        return value.IsList();
    }
    if (lowered_type_name == "tuple") {
        return value.IsTuple();
    }
    if (lowered_type_name == "set") {
        return value.IsSet();
    }
    if (lowered_type_name == "map") {
        return value.IsMap();
    }
    if (lowered_type_name == "function") {
        return value.IsFunctionRef();
    }
    if (lowered_type_name == "object") {
        return value.IsObject();
    }
    return false;
}

std::uint64_t HashMix(std::uint64_t seed, std::uint64_t value) {
    seed ^= value + 0x9E3779B97F4A7C15ULL + (seed << 6U) + (seed >> 2U);
    return seed;
}

std::uint64_t HashBytes(std::string_view text) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char ch : text) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::uint64_t HashValue(const runtime::Value& value, int depth = 0) {
    if (depth > 128) {
        return 0xDEADBEEFCAFEBABEULL;
    }

    if (value.IsNull()) {
        return 0x0100000000000000ULL;
    }

    if (value.IsBool()) {
        return value.AsBool() ? 0x0200000000000001ULL : 0x0200000000000000ULL;
    }

    if (value.IsNumber()) {
        BigInt integer;
        if (value.AsBigInt(&integer)) {
            std::uint64_t hash = 0x0300000000000000ULL;
            hash = HashMix(hash, HashBytes(integer.convert_to<std::string>()));
            return hash;
        }

        runtime::Value::Decimal decimal;
        if (value.AsDecimal(&decimal)) {
            std::uint64_t hash = 0x0300000000000001ULL;
            hash = HashMix(hash, HashBytes(decimal.ToString()));
            return hash;
        }

        bool ok = false;
        const double numeric = value.AsNumber(&ok);
        std::uint64_t hash = 0x0300000000000002ULL;
        if (ok) {
            std::uint64_t bits = 0;
            std::memcpy(&bits, &numeric, sizeof(bits));
            hash = HashMix(hash, bits);
        }
        return hash;
    }

    if (value.IsChar()) {
        const char character = *value.AsCharValue();
        std::uint64_t hash = 0x0400000000000000ULL;
        hash = HashMix(hash, static_cast<std::uint64_t>(static_cast<unsigned char>(character)));
        return hash;
    }

    if (value.IsString()) {
        std::uint64_t hash = 0x0500000000000000ULL;
        hash = HashMix(hash, HashBytes(value.ToString()));
        return hash;
    }

    if (const auto* list = value.AsList()) {
        std::uint64_t hash = 0x0600000000000000ULL;
        for (const auto& element : *list) {
            hash = HashMix(hash, HashValue(element, depth + 1));
        }
        return hash;
    }

    if (const auto* tuple = value.AsTuple()) {
        std::uint64_t hash = 0x0700000000000000ULL;
        for (const auto& element : *tuple) {
            hash = HashMix(hash, HashValue(element, depth + 1));
        }
        return hash;
    }

    if (const auto* set = value.AsSet()) {
        std::vector<std::uint64_t> members;
        members.reserve(set->size());
        for (const auto& element : *set) {
            members.push_back(HashValue(element, depth + 1));
        }
        std::sort(members.begin(), members.end());

        std::uint64_t hash = 0x0800000000000000ULL;
        for (const std::uint64_t member_hash : members) {
            hash = HashMix(hash, member_hash);
        }
        return hash;
    }

    if (const auto* map = value.AsMap()) {
        std::vector<std::uint64_t> entries;
        entries.reserve(map->size());
        for (const auto& entry : *map) {
            std::uint64_t entry_hash = 0x0900000000000000ULL;
            entry_hash = HashMix(entry_hash, HashValue(entry.first, depth + 1));
            entry_hash = HashMix(entry_hash, HashValue(entry.second, depth + 1));
            entries.push_back(entry_hash);
        }
        std::sort(entries.begin(), entries.end());

        std::uint64_t hash = 0x0900000000000001ULL;
        for (const std::uint64_t entry_hash : entries) {
            hash = HashMix(hash, entry_hash);
        }
        return hash;
    }

    if (const auto* object = value.AsObject()) {
        std::uint64_t hash = 0x0A00000000000000ULL;
        for (const auto& entry : *object) {
            hash = HashMix(hash, HashBytes(entry.first));
            hash = HashMix(hash, HashValue(entry.second, depth + 1));
        }
        return hash;
    }

    const runtime::Value::FunctionRef* function = value.AsFunctionRefValue();
    std::uint64_t hash = 0x0B00000000000000ULL;
    hash = HashMix(hash, HashBytes(function != nullptr ? function->name : value.ToString()));
    return hash;
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

    if (call.callee == "len") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "len(value) requiere 1 argumento.";
            return false;
        }

        runtime::Value value;
        if (!evaluate_argument(0, &value)) {
            return false;
        }

        std::size_t size = 0;
        if (const auto* text = value.AsCharValue()) {
            (void)text;
            size = 1;
        } else if (value.IsString()) {
            size = value.ToString().size();
        } else if (const auto* list = value.AsList()) {
            size = list->size();
        } else if (const auto* tuple = value.AsTuple()) {
            size = tuple->size();
        } else if (const auto* set = value.AsSet()) {
            size = set->size();
        } else if (const auto* map = value.AsMap()) {
            size = map->size();
        } else if (const auto* object = value.AsObject()) {
            size = object->size();
        } else {
            *out_error = "len() requiere un string, list, tuple, set, map, object o char.";
            return false;
        }

        *out_value = runtime::Value(SizeToBigInt(size));
        return true;
    }

    if (call.callee == "range") {
        *out_was_builtin = true;
        if (call.arguments.empty() || call.arguments.size() > 3) {
            *out_error = "range() requiere 1, 2 o 3 argumentos.";
            return false;
        }

        BigInt start = 0;
        BigInt stop = 0;
        BigInt step = 1;

        runtime::Value first_value;
        BigInt first_integer;
        if (!evaluate_argument(0, &first_value) || !ReadInteger(first_value, &first_integer, out_error)) {
            return false;
        }

        if (call.arguments.size() == 1) {
            stop = first_integer;
        } else {
            start = first_integer;

            runtime::Value second;
            if (!evaluate_argument(1, &second) || !ReadInteger(second, &stop, out_error)) {
                return false;
            }

            if (call.arguments.size() == 3) {
                runtime::Value third;
                if (!evaluate_argument(2, &third) || !ReadInteger(third, &step, out_error)) {
                    return false;
                }
            }
        }

        if (step == 0) {
            *out_error = "range() requiere step != 0.";
            return false;
        }

        static constexpr std::size_t kMaxRangeElements = 1000000;
        runtime::Value::List values;
        BigInt current = start;
        while ((step > 0 && current < stop) || (step < 0 && current > stop)) {
            if (values.size() >= kMaxRangeElements) {
                *out_error = "range() excede el maximo de 1000000 elementos.";
                return false;
            }
            values.emplace_back(current);
            current += step;
        }

        *out_value = runtime::Value(std::move(values));
        return true;
    }

    if (call.callee == "enumerate") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1 && call.arguments.size() != 2) {
            *out_error = "enumerate(iterable, start=0) requiere 1 o 2 argumentos.";
            return false;
        }

        runtime::Value iterable;
        if (!evaluate_argument(0, &iterable)) {
            return false;
        }

        std::vector<runtime::Value> elements;
        std::string iterable_error;
        if (!CollectForEachElements(iterable, &elements, &iterable_error)) {
            *out_error = "enumerate() requiere un iterable (list, tuple, set, map, object o string).";
            return false;
        }

        BigInt index = 0;
        if (call.arguments.size() == 2) {
            runtime::Value start_value;
            if (!evaluate_argument(1, &start_value) || !ReadInteger(start_value, &index, out_error)) {
                return false;
            }
        }

        runtime::Value::List result;
        result.reserve(elements.size());
        for (const auto& element : elements) {
            runtime::Value::Tuple pair;
            pair.elements.push_back(runtime::Value(index));
            pair.elements.push_back(element);
            result.push_back(runtime::Value(std::move(pair)));
            index += 1;
        }

        *out_value = runtime::Value(std::move(result));
        return true;
    }

    if (call.callee == "zip") {
        *out_was_builtin = true;
        runtime::Value::List result;
        if (call.arguments.empty()) {
            *out_value = runtime::Value(std::move(result));
            return true;
        }

        std::vector<std::vector<runtime::Value>> collections;
        collections.reserve(call.arguments.size());
        std::size_t min_size = std::numeric_limits<std::size_t>::max();

        for (std::size_t i = 0; i < call.arguments.size(); ++i) {
            runtime::Value source;
            if (!evaluate_argument(i, &source)) {
                return false;
            }

            std::vector<runtime::Value> elements;
            std::string iterable_error;
            if (!CollectForEachElements(source, &elements, &iterable_error)) {
                *out_error = "zip() requiere iterables validos en todos los argumentos.";
                return false;
            }

            min_size = std::min(min_size, elements.size());
            collections.push_back(std::move(elements));
        }

        result.reserve(min_size);
        for (std::size_t row = 0; row < min_size; ++row) {
            runtime::Value::Tuple tuple_value;
            tuple_value.elements.reserve(collections.size());
            for (const auto& collection : collections) {
                tuple_value.elements.push_back(collection[row]);
            }
            result.push_back(runtime::Value(std::move(tuple_value)));
        }

        *out_value = runtime::Value(std::move(result));
        return true;
    }

    if (call.callee == "all" || call.callee == "any") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = call.callee + "(iterable) requiere 1 argumento.";
            return false;
        }

        runtime::Value iterable;
        if (!evaluate_argument(0, &iterable)) {
            return false;
        }

        std::vector<runtime::Value> elements;
        std::string iterable_error;
        if (!CollectForEachElements(iterable, &elements, &iterable_error)) {
            *out_error = call.callee + "() requiere un iterable (list, tuple, set, map, object o string).";
            return false;
        }

        if (call.callee == "all") {
            for (const auto& element : elements) {
                if (!element.AsBool()) {
                    *out_value = runtime::Value(false);
                    return true;
                }
            }
            *out_value = runtime::Value(true);
            return true;
        }

        for (const auto& element : elements) {
            if (element.AsBool()) {
                *out_value = runtime::Value(true);
                return true;
            }
        }
        *out_value = runtime::Value(false);
        return true;
    }

    if (call.callee == "isinstance") {
        *out_was_builtin = true;
        if (call.arguments.size() != 2) {
            *out_error = "isinstance(value, type_name) requiere 2 argumentos.";
            return false;
        }

        runtime::Value value;
        runtime::Value type_spec;
        if (!evaluate_argument(0, &value) || !evaluate_argument(1, &type_spec)) {
            return false;
        }

        auto matches_type_name = [&](const std::string& type_name, bool* out_match) -> bool {
            const std::string lowered = LowerAscii(type_name);
            if (MatchesBuiltinTypeName(value, lowered)) {
                *out_match = true;
                return true;
            }

            std::string class_name;
            *out_match = IsClassInstance(value, &class_name) && IsClassTypeOrDerived(class_name, type_name);
            return true;
        };

        std::function<bool(const runtime::Value&, bool*)> matches_type_spec;
        matches_type_spec = [&](const runtime::Value& candidate, bool* out_match) -> bool {
            if (candidate.IsString() || candidate.IsChar()) {
                return matches_type_name(candidate.ToString(), out_match);
            }

            const std::vector<runtime::Value>* collection = nullptr;
            if (const auto* list = candidate.AsList()) {
                collection = list;
            } else if (const auto* tuple = candidate.AsTuple()) {
                collection = tuple;
            } else if (const auto* set = candidate.AsSet()) {
                collection = set;
            }

            if (collection != nullptr) {
                for (const auto& nested : *collection) {
                    bool nested_match = false;
                    if (!matches_type_spec(nested, &nested_match)) {
                        return false;
                    }
                    if (nested_match) {
                        *out_match = true;
                        return true;
                    }
                }
                *out_match = false;
                return true;
            }

            *out_error = "isinstance(): type_name debe ser string, char, list, tuple o set.";
            return false;
        };

        bool match = false;
        if (!matches_type_spec(type_spec, &match)) {
            return false;
        }

        *out_value = runtime::Value(match);
        return true;
    }

    if (call.callee == "chr") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "chr(code) requiere 1 argumento.";
            return false;
        }

        runtime::Value input;
        if (!evaluate_argument(0, &input)) {
            return false;
        }

        BigInt code;
        if (!ReadInteger(input, &code, out_error)) {
            return false;
        }
        if (code < 0 || code > 255) {
            *out_error = "chr(code) requiere 0 <= code <= 255.";
            return false;
        }

        const unsigned char ascii = static_cast<unsigned char>(code.convert_to<unsigned>());
        *out_value = runtime::Value(static_cast<char>(ascii));
        return true;
    }

    if (call.callee == "ord") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "ord(char) requiere 1 argumento.";
            return false;
        }

        runtime::Value input;
        if (!evaluate_argument(0, &input)) {
            return false;
        }

        unsigned char ascii = 0;
        if (input.IsChar()) {
            ascii = static_cast<unsigned char>(*input.AsCharValue());
        } else if (input.IsString()) {
            const std::string text = input.ToString();
            if (text.size() != 1) {
                *out_error = "ord() requiere char o string de longitud 1.";
                return false;
            }
            ascii = static_cast<unsigned char>(text[0]);
        } else {
            *out_error = "ord() requiere char o string de longitud 1.";
            return false;
        }

        *out_value = runtime::Value(BigInt(ascii));
        return true;
    }

    if (call.callee == "hex" || call.callee == "bin") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = call.callee + "(value) requiere 1 argumento.";
            return false;
        }

        runtime::Value input;
        if (!evaluate_argument(0, &input)) {
            return false;
        }

        BigInt integer;
        if (!ReadInteger(input, &integer, out_error)) {
            return false;
        }

        const bool negative = integer < 0;
        const BigInt absolute = AbsBigInt(integer);
        const std::string digits = call.callee == "hex" ? BigIntToHexString(absolute, false) : BigIntToBinaryString(absolute);
        const std::string prefix = call.callee == "hex" ? "0x" : "0b";

        *out_value = runtime::Value((negative ? "-" : "") + prefix + digits);
        return true;
    }

    if (call.callee == "hash") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "hash(value) requiere 1 argumento.";
            return false;
        }

        runtime::Value input;
        if (!evaluate_argument(0, &input)) {
            return false;
        }

        *out_value = runtime::Value(UnsignedToBigInt(HashValue(input)));
        return true;
    }

    if (call.callee == "id") {
        *out_was_builtin = true;
        if (call.arguments.size() != 1) {
            *out_error = "id(value) requiere 1 argumento.";
            return false;
        }

        runtime::Value input;
        if (!evaluate_argument(0, &input)) {
            return false;
        }

        const std::uint64_t fingerprint = HashValue(input);
        auto& bucket = value_identity_cache_[fingerprint];
        for (const auto& entry : bucket) {
            if (entry.first.Equals(input)) {
                *out_value = runtime::Value(entry.second);
                return true;
            }
        }

        if (next_value_identity_id_ == std::numeric_limits<long long>::max()) {
            *out_error = "id() excedio el limite de identidades en runtime.";
            return false;
        }

        const long long assigned_id = next_value_identity_id_++;
        bucket.push_back({input, assigned_id});
        *out_value = runtime::Value(assigned_id);
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

        *out_value = runtime::Value(ValueTypeName(value));
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
