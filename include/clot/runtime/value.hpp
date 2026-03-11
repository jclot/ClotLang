#ifndef CLOT_RUNTIME_VALUE_HPP
#define CLOT_RUNTIME_VALUE_HPP

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "clot/runtime/bigint.hpp"
#include "clot/runtime/decimal.hpp"

namespace clot::runtime {

enum class VariableKind {
    Dynamic,
    Int,
    Double,
    Float,
    Decimal,
    Long,
    Byte,
    Char,
    List,
    Tuple,
    Set,
    Map,
    Object,
    Function,
};

class Value {
public:
    using BigInt = clot::runtime::BigInt;
    using Decimal = clot::runtime::Decimal;
    using List = std::vector<Value>;
    using Object = std::vector<std::pair<std::string, Value>>;

    struct Tuple {
        std::vector<Value> elements;
    };

    struct Set {
        std::vector<Value> elements;
    };

    struct Map {
        std::vector<std::pair<Value, Value>> entries;
    };

    struct FunctionRef {
        std::string name;
    };

    Value() : data_(BigInt(0)) {}
    Value(std::nullptr_t) : data_(std::monostate{}) {}
    explicit Value(BigInt value) : data_(std::move(value)) {}
    explicit Value(long long value) : data_(BigInt(value)) {}
    explicit Value(double value) : data_(value) {}
    explicit Value(float value) : data_(value) {}
    explicit Value(Decimal value) : data_(std::move(value)) {}
    explicit Value(char value) : data_(value) {}
    explicit Value(std::string value) : data_(std::move(value)) {}
    explicit Value(const char* value) : data_(std::string(value)) {}
    explicit Value(bool value) : data_(value) {}
    explicit Value(List value) : data_(std::move(value)) {}
    explicit Value(Object value) : data_(std::move(value)) {}
    explicit Value(Tuple value) : data_(std::move(value)) {}
    explicit Value(Set value) : data_(std::move(value)) {}
    explicit Value(Map value) : data_(std::move(value)) {}
    explicit Value(FunctionRef value) : data_(std::move(value)) {}

    bool IsNull() const { return std::holds_alternative<std::monostate>(data_); }
    bool IsNumber() const {
        return std::holds_alternative<BigInt>(data_) ||
               std::holds_alternative<double>(data_) ||
               std::holds_alternative<float>(data_) ||
               std::holds_alternative<Decimal>(data_);
    }
    bool IsInteger() const { return std::holds_alternative<BigInt>(data_); }
    bool IsDouble() const { return std::holds_alternative<double>(data_); }
    bool IsFloat() const { return std::holds_alternative<float>(data_); }
    bool IsDecimal() const { return std::holds_alternative<Decimal>(data_); }
    bool IsChar() const { return std::holds_alternative<char>(data_); }
    bool IsString() const { return std::holds_alternative<std::string>(data_); }
    bool IsBool() const { return std::holds_alternative<bool>(data_); }
    bool IsList() const { return std::holds_alternative<List>(data_); }
    bool IsTuple() const { return std::holds_alternative<Tuple>(data_); }
    bool IsSet() const { return std::holds_alternative<Set>(data_); }
    bool IsMap() const { return std::holds_alternative<Map>(data_); }
    bool IsObject() const { return std::holds_alternative<Object>(data_); }
    bool IsFunctionRef() const { return std::holds_alternative<FunctionRef>(data_); }

    const BigInt* AsBigIntValue() const {
        if (!IsInteger()) {
            return nullptr;
        }
        return &std::get<BigInt>(data_);
    }

    const Decimal* AsDecimalValue() const {
        if (!IsDecimal()) {
            return nullptr;
        }
        return &std::get<Decimal>(data_);
    }

    const char* AsCharValue() const {
        if (!IsChar()) {
            return nullptr;
        }
        return &std::get<char>(data_);
    }

    const FunctionRef* AsFunctionRefValue() const {
        if (!IsFunctionRef()) {
            return nullptr;
        }
        return &std::get<FunctionRef>(data_);
    }

    bool AsBigInt(BigInt* out_integer) const {
        if (out_integer == nullptr) {
            return false;
        }

        if (std::holds_alternative<BigInt>(data_)) {
            *out_integer = std::get<BigInt>(data_);
            return true;
        }

        if (std::holds_alternative<double>(data_)) {
            const double value = std::get<double>(data_);
            if (!std::isfinite(value) || std::trunc(value) != value) {
                return false;
            }
            std::ostringstream stream;
            stream << std::fixed << std::setprecision(0) << value;
            return TryParseBigInt(stream.str(), out_integer);
        }

        if (std::holds_alternative<float>(data_)) {
            const float value = std::get<float>(data_);
            if (!std::isfinite(value) || std::trunc(value) != value) {
                return false;
            }
            std::ostringstream stream;
            stream << std::fixed << std::setprecision(0) << value;
            return TryParseBigInt(stream.str(), out_integer);
        }

        if (std::holds_alternative<Decimal>(data_)) {
            return std::get<Decimal>(data_).ToBigIntIfIntegral(out_integer);
        }

        if (std::holds_alternative<std::string>(data_)) {
            return TryParseBigInt(std::get<std::string>(data_), out_integer);
        }

        if (std::holds_alternative<bool>(data_)) {
            *out_integer = std::get<bool>(data_) ? BigInt(1) : BigInt(0);
            return true;
        }

        if (std::holds_alternative<char>(data_)) {
            *out_integer = static_cast<unsigned char>(std::get<char>(data_));
            return true;
        }

        return false;
    }

    bool AsDecimal(Decimal* out_decimal) const {
        if (out_decimal == nullptr) {
            return false;
        }

        if (std::holds_alternative<Decimal>(data_)) {
            *out_decimal = std::get<Decimal>(data_);
            return true;
        }

        if (std::holds_alternative<BigInt>(data_)) {
            *out_decimal = Decimal::FromBigInt(std::get<BigInt>(data_));
            return true;
        }

        if (std::holds_alternative<double>(data_)) {
            std::ostringstream stream;
            stream << std::setprecision(17) << std::get<double>(data_);
            return Decimal::TryParse(stream.str(), out_decimal);
        }

        if (std::holds_alternative<float>(data_)) {
            std::ostringstream stream;
            stream << std::setprecision(9) << std::get<float>(data_);
            return Decimal::TryParse(stream.str(), out_decimal);
        }

        if (std::holds_alternative<std::string>(data_)) {
            return Decimal::TryParse(std::get<std::string>(data_), out_decimal);
        }

        if (std::holds_alternative<bool>(data_)) {
            *out_decimal = Decimal::FromBigInt(std::get<bool>(data_) ? BigInt(1) : BigInt(0));
            return true;
        }

        if (std::holds_alternative<char>(data_)) {
            *out_decimal = Decimal::FromBigInt(static_cast<unsigned char>(std::get<char>(data_)));
            return true;
        }

        return false;
    }

    const List* AsList() const {
        if (!IsList()) {
            return nullptr;
        }
        return &std::get<List>(data_);
    }

    List* MutableList() {
        if (!IsList()) {
            return nullptr;
        }
        return &std::get<List>(data_);
    }

    const std::vector<Value>* AsTuple() const {
        if (!IsTuple()) {
            return nullptr;
        }
        return &std::get<Tuple>(data_).elements;
    }

    const std::vector<Value>* AsSet() const {
        if (!IsSet()) {
            return nullptr;
        }
        return &std::get<Set>(data_).elements;
    }

    std::vector<Value>* MutableSet() {
        if (!IsSet()) {
            return nullptr;
        }
        return &std::get<Set>(data_).elements;
    }

    const std::vector<std::pair<Value, Value>>* AsMap() const {
        if (!IsMap()) {
            return nullptr;
        }
        return &std::get<Map>(data_).entries;
    }

    std::vector<std::pair<Value, Value>>* MutableMap() {
        if (!IsMap()) {
            return nullptr;
        }
        return &std::get<Map>(data_).entries;
    }

    const Value* GetMapValue(const Value& key) const {
        const auto* map = AsMap();
        if (map == nullptr) {
            return nullptr;
        }
        for (const auto& entry : *map) {
            if (entry.first.Equals(key)) {
                return &entry.second;
            }
        }
        return nullptr;
    }

    Value* GetMutableMapValue(const Value& key) {
        auto* map = MutableMap();
        if (map == nullptr) {
            return nullptr;
        }
        for (auto& entry : *map) {
            if (entry.first.Equals(key)) {
                return &entry.second;
            }
        }
        return nullptr;
    }

    Value* EnsureMapValue(const Value& key) {
        auto* map = MutableMap();
        if (map == nullptr) {
            return nullptr;
        }
        for (auto& entry : *map) {
            if (entry.first.Equals(key)) {
                return &entry.second;
            }
        }
        map->push_back({key, Value(nullptr)});
        return &map->back().second;
    }

    const Object* AsObject() const {
        if (!IsObject()) {
            return nullptr;
        }
        return &std::get<Object>(data_);
    }

    Object* MutableObject() {
        if (!IsObject()) {
            return nullptr;
        }
        return &std::get<Object>(data_);
    }

    const Value* GetObjectProperty(const std::string& key) const {
        if (!IsObject()) {
            return nullptr;
        }

        const Object& object = std::get<Object>(data_);
        for (const auto& entry : object) {
            if (entry.first == key) {
                return &entry.second;
            }
        }

        return nullptr;
    }

    Value* GetMutableObjectProperty(const std::string& key) {
        if (!IsObject()) {
            return nullptr;
        }

        Object& object = std::get<Object>(data_);
        for (auto& entry : object) {
            if (entry.first == key) {
                return &entry.second;
            }
        }

        return nullptr;
    }

    Value* EnsureObjectProperty(const std::string& key) {
        if (!IsObject()) {
            return nullptr;
        }

        Object& object = std::get<Object>(data_);
        for (auto& entry : object) {
            if (entry.first == key) {
                return &entry.second;
            }
        }

        object.push_back({key, Value(nullptr)});
        return &object.back().second;
    }

    double AsNumber(bool* out_ok = nullptr) const {
        if (std::holds_alternative<BigInt>(data_)) {
            try {
                if (out_ok != nullptr) {
                    *out_ok = true;
                }
                return std::get<BigInt>(data_).convert_to<double>();
            } catch (...) {
                if (out_ok != nullptr) {
                    *out_ok = false;
                }
                return 0.0;
            }
        }

        if (std::holds_alternative<double>(data_)) {
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return std::get<double>(data_);
        }

        if (std::holds_alternative<float>(data_)) {
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return static_cast<double>(std::get<float>(data_));
        }

        if (std::holds_alternative<Decimal>(data_)) {
            double value = 0.0;
            const bool ok = std::get<Decimal>(data_).ToDouble(&value);
            if (out_ok != nullptr) {
                *out_ok = ok;
            }
            return ok ? value : 0.0;
        }

        if (std::holds_alternative<bool>(data_)) {
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return std::get<bool>(data_) ? 1.0 : 0.0;
        }

        if (std::holds_alternative<char>(data_)) {
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return static_cast<unsigned char>(std::get<char>(data_));
        }

        if (std::holds_alternative<std::string>(data_)) {
            const std::string& text = std::get<std::string>(data_);
            try {
                std::size_t consumed = 0;
                const double numeric = std::stod(text, &consumed);
                if (consumed != text.size()) {
                    if (out_ok != nullptr) {
                        *out_ok = false;
                    }
                    return 0.0;
                }

                if (out_ok != nullptr) {
                    *out_ok = true;
                }
                return numeric;
            } catch (...) {
                if (out_ok != nullptr) {
                    *out_ok = false;
                }
                return 0.0;
            }
        }

        if (out_ok != nullptr) {
            *out_ok = false;
        }
        return 0.0;
    }

    long long AsInteger(bool* out_ok = nullptr) const {
        BigInt value;
        if (!AsBigInt(&value)) {
            if (out_ok != nullptr) {
                *out_ok = false;
            }
            return 0;
        }

        long long result = 0;
        if (!TryBigIntToInt64(value, &result)) {
            if (out_ok != nullptr) {
                *out_ok = false;
            }
            return 0;
        }

        if (out_ok != nullptr) {
            *out_ok = true;
        }
        return result;
    }

    bool AsBool() const {
        if (std::holds_alternative<std::monostate>(data_)) {
            return false;
        }

        if (std::holds_alternative<bool>(data_)) {
            return std::get<bool>(data_);
        }

        if (std::holds_alternative<BigInt>(data_)) {
            return std::get<BigInt>(data_) != 0;
        }

        if (std::holds_alternative<double>(data_)) {
            return std::get<double>(data_) != 0.0;
        }

        if (std::holds_alternative<float>(data_)) {
            return std::get<float>(data_) != 0.0F;
        }

        if (std::holds_alternative<Decimal>(data_)) {
            return std::get<Decimal>(data_).Coefficient() != 0;
        }

        if (std::holds_alternative<char>(data_)) {
            return std::get<char>(data_) != '\0';
        }

        if (std::holds_alternative<std::string>(data_)) {
            return !std::get<std::string>(data_).empty();
        }

        if (std::holds_alternative<List>(data_)) {
            return !std::get<List>(data_).empty();
        }

        if (std::holds_alternative<Tuple>(data_)) {
            return !std::get<Tuple>(data_).elements.empty();
        }

        if (std::holds_alternative<Set>(data_)) {
            return !std::get<Set>(data_).elements.empty();
        }

        if (std::holds_alternative<Map>(data_)) {
            return !std::get<Map>(data_).entries.empty();
        }

        if (std::holds_alternative<Object>(data_)) {
            return !std::get<Object>(data_).empty();
        }

        return true;
    }

    bool Equals(const Value& other) const {
        if (IsNull() || other.IsNull()) {
            return IsNull() && other.IsNull();
        }

        if (IsNumber() && other.IsNumber()) {
            if (IsDecimal() || other.IsDecimal()) {
                Decimal lhs_decimal;
                Decimal rhs_decimal;
                return AsDecimal(&lhs_decimal) && other.AsDecimal(&rhs_decimal) && lhs_decimal == rhs_decimal;
            }

            BigInt lhs_integer;
            BigInt rhs_integer;
            const bool lhs_is_integer = AsBigInt(&lhs_integer);
            const bool rhs_is_integer = other.AsBigInt(&rhs_integer);
            if (lhs_is_integer && rhs_is_integer) {
                return lhs_integer == rhs_integer;
            }

            bool lhs_ok = false;
            bool rhs_ok = false;
            const double lhs_number = AsNumber(&lhs_ok);
            const double rhs_number = other.AsNumber(&rhs_ok);
            return lhs_ok && rhs_ok && lhs_number == rhs_number;
        }

        if (IsChar() && other.IsChar()) {
            return std::get<char>(data_) == std::get<char>(other.data_);
        }
        if (IsString() && other.IsString()) {
            return std::get<std::string>(data_) == std::get<std::string>(other.data_);
        }
        if (IsBool() && other.IsBool()) {
            return std::get<bool>(data_) == std::get<bool>(other.data_);
        }

        if (IsList() && other.IsList()) {
            return SequenceEquals(std::get<List>(data_), std::get<List>(other.data_));
        }

        if (IsTuple() && other.IsTuple()) {
            return SequenceEquals(std::get<Tuple>(data_).elements, std::get<Tuple>(other.data_).elements);
        }

        if (IsSet() && other.IsSet()) {
            const auto& lhs_set = std::get<Set>(data_).elements;
            const auto& rhs_set = std::get<Set>(other.data_).elements;
            if (lhs_set.size() != rhs_set.size()) {
                return false;
            }
            std::vector<bool> matched(rhs_set.size(), false);
            for (const auto& left_value : lhs_set) {
                bool found = false;
                for (std::size_t i = 0; i < rhs_set.size(); ++i) {
                    if (!matched[i] && left_value.Equals(rhs_set[i])) {
                        matched[i] = true;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true;
        }

        if (IsMap() && other.IsMap()) {
            const auto& lhs_map = std::get<Map>(data_).entries;
            const auto& rhs_map = std::get<Map>(other.data_).entries;
            if (lhs_map.size() != rhs_map.size()) {
                return false;
            }
            std::vector<bool> matched(rhs_map.size(), false);
            for (const auto& left_entry : lhs_map) {
                bool found = false;
                for (std::size_t i = 0; i < rhs_map.size(); ++i) {
                    if (matched[i]) {
                        continue;
                    }
                    if (left_entry.first.Equals(rhs_map[i].first) && left_entry.second.Equals(rhs_map[i].second)) {
                        matched[i] = true;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    return false;
                }
            }
            return true;
        }

        if (IsObject() && other.IsObject()) {
            const Object& lhs_object = std::get<Object>(data_);
            const Object& rhs_object = std::get<Object>(other.data_);
            if (lhs_object.size() != rhs_object.size()) {
                return false;
            }
            for (std::size_t i = 0; i < lhs_object.size(); ++i) {
                if (lhs_object[i].first != rhs_object[i].first || !lhs_object[i].second.Equals(rhs_object[i].second)) {
                    return false;
                }
            }
            return true;
        }

        if (IsFunctionRef() && other.IsFunctionRef()) {
            return std::get<FunctionRef>(data_).name == std::get<FunctionRef>(other.data_).name;
        }

        return false;
    }

    std::string ToString() const {
        return ToStringInternal(false);
    }

    static bool TryParseBigInt(const std::string& text, BigInt* out_integer) {
        return BigInt::TryParse(text, out_integer);
    }

    static bool TryBigIntToInt64(const BigInt& value, long long* out_integer) {
        return value.ToLongLong(out_integer);
    }

private:
    static bool SequenceEquals(const std::vector<Value>& lhs, const std::vector<Value>& rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (std::size_t i = 0; i < lhs.size(); ++i) {
            if (!lhs[i].Equals(rhs[i])) {
                return false;
            }
        }
        return true;
    }

    static std::string FormatNumber(double value) {
        std::ostringstream stream;
        stream << std::setprecision(15) << value;
        std::string text = stream.str();

        const std::size_t dot = text.find('.');
        if (dot != std::string::npos) {
            while (!text.empty() && text.back() == '0') {
                text.pop_back();
            }
            if (!text.empty() && text.back() == '.') {
                text.pop_back();
            }
        }

        if (text.empty()) {
            return "0";
        }

        return text;
    }

    static std::string FormatFloat(float value) {
        std::ostringstream stream;
        stream << std::setprecision(7) << value;
        std::string text = stream.str();

        const std::size_t dot = text.find('.');
        if (dot != std::string::npos) {
            while (!text.empty() && text.back() == '0') {
                text.pop_back();
            }
            if (!text.empty() && text.back() == '.') {
                text.pop_back();
            }
        }

        if (text.empty()) {
            return "0";
        }

        return text;
    }

    static std::string EscapeString(const std::string& text) {
        std::string escaped;
        escaped.reserve(text.size());

        for (char character : text) {
            if (character == '\\') {
                escaped += "\\\\";
            } else if (character == '"') {
                escaped += "\\\"";
            } else if (character == '\n') {
                escaped += "\\n";
            } else if (character == '\t') {
                escaped += "\\t";
            } else if (character == '\r') {
                escaped += "\\r";
            } else {
                escaped.push_back(character);
            }
        }

        return escaped;
    }

    static std::string EscapeChar(char character) {
        if (character == '\\') {
            return "\\\\";
        }
        if (character == '\'') {
            return "\\'";
        }
        if (character == '\n') {
            return "\\n";
        }
        if (character == '\t') {
            return "\\t";
        }
        if (character == '\r') {
            return "\\r";
        }
        if (character == '\0') {
            return "\\0";
        }
        return std::string(1, character);
    }

    std::string ToStringInternal(bool quote_string) const {
        if (std::holds_alternative<std::monostate>(data_)) {
            return "null";
        }

        if (std::holds_alternative<std::string>(data_)) {
            const std::string& raw = std::get<std::string>(data_);
            if (!quote_string) {
                return raw;
            }
            return "\"" + EscapeString(raw) + "\"";
        }

        if (std::holds_alternative<char>(data_)) {
            const char value = std::get<char>(data_);
            if (!quote_string) {
                return std::string(1, value);
            }
            return "'" + EscapeChar(value) + "'";
        }

        if (std::holds_alternative<bool>(data_)) {
            return std::get<bool>(data_) ? "true" : "false";
        }

        if (std::holds_alternative<BigInt>(data_)) {
            return std::get<BigInt>(data_).convert_to<std::string>();
        }

        if (std::holds_alternative<double>(data_)) {
            return FormatNumber(std::get<double>(data_));
        }

        if (std::holds_alternative<float>(data_)) {
            return FormatFloat(std::get<float>(data_));
        }

        if (std::holds_alternative<Decimal>(data_)) {
            return std::get<Decimal>(data_).ToString();
        }

        if (std::holds_alternative<List>(data_)) {
            const List& list = std::get<List>(data_);
            std::string text = "[";
            for (std::size_t i = 0; i < list.size(); ++i) {
                text += list[i].ToStringInternal(true);
                if (i + 1 < list.size()) {
                    text += ", ";
                }
            }
            text += "]";
            return text;
        }

        if (std::holds_alternative<Tuple>(data_)) {
            const auto& tuple = std::get<Tuple>(data_).elements;
            std::string text = "(";
            for (std::size_t i = 0; i < tuple.size(); ++i) {
                text += tuple[i].ToStringInternal(true);
                if (i + 1 < tuple.size()) {
                    text += ", ";
                }
            }
            if (tuple.size() == 1) {
                text += ",";
            }
            text += ")";
            return text;
        }

        if (std::holds_alternative<Set>(data_)) {
            const auto& set = std::get<Set>(data_).elements;
            std::string text = "set{";
            for (std::size_t i = 0; i < set.size(); ++i) {
                text += set[i].ToStringInternal(true);
                if (i + 1 < set.size()) {
                    text += ", ";
                }
            }
            text += "}";
            return text;
        }

        if (std::holds_alternative<Map>(data_)) {
            const auto& map = std::get<Map>(data_).entries;
            std::string text = "map{";
            for (std::size_t i = 0; i < map.size(); ++i) {
                text += map[i].first.ToStringInternal(true);
                text += ": ";
                text += map[i].second.ToStringInternal(true);
                if (i + 1 < map.size()) {
                    text += ", ";
                }
            }
            text += "}";
            return text;
        }

        if (std::holds_alternative<Object>(data_)) {
            const Object& object = std::get<Object>(data_);
            std::string text = "{";
            for (std::size_t i = 0; i < object.size(); ++i) {
                text += object[i].first;
                text += ": ";
                text += object[i].second.ToStringInternal(true);
                if (i + 1 < object.size()) {
                    text += ", ";
                }
            }
            text += "}";
            return text;
        }

        const FunctionRef& function = std::get<FunctionRef>(data_);
        return "<function:" + function.name + ">";
    }

    std::variant<
        std::monostate,
        BigInt,
        double,
        float,
        Decimal,
        char,
        std::string,
        bool,
        List,
        Tuple,
        Set,
        Map,
        Object,
        FunctionRef>
        data_;
};

struct VariableSlot {
    Value value;
    VariableKind kind = VariableKind::Dynamic;
    bool is_const = false;
};

}  // namespace clot::runtime

#endif  // CLOT_RUNTIME_VALUE_HPP
