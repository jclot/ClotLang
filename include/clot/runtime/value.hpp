#ifndef CLOT_RUNTIME_VALUE_HPP
#define CLOT_RUNTIME_VALUE_HPP

#include <algorithm>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace clot::runtime {

enum class VariableKind {
    Dynamic,
    Long,
    Byte,
};

class Value {
public:
    using List = std::vector<Value>;
    using Object = std::vector<std::pair<std::string, Value>>;

    Value() : data_(0.0) {}
    explicit Value(long long value) : data_(value) {}
    explicit Value(double value) : data_(value) {}
    explicit Value(std::string value) : data_(std::move(value)) {}
    explicit Value(const char* value) : data_(std::string(value)) {}
    explicit Value(bool value) : data_(value) {}
    explicit Value(List value) : data_(std::move(value)) {}
    explicit Value(Object value) : data_(std::move(value)) {}

    bool IsNumber() const {
        return std::holds_alternative<long long>(data_) || std::holds_alternative<double>(data_);
    }
    bool IsInteger() const { return std::holds_alternative<long long>(data_); }
    bool IsString() const { return std::holds_alternative<std::string>(data_); }
    bool IsBool() const { return std::holds_alternative<bool>(data_); }
    bool IsList() const { return std::holds_alternative<List>(data_); }
    bool IsObject() const { return std::holds_alternative<Object>(data_); }

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
        for (const auto& [entry_key, entry_value] : object) {
            if (entry_key == key) {
                return &entry_value;
            }
        }

        return nullptr;
    }

    Value* GetMutableObjectProperty(const std::string& key) {
        if (!IsObject()) {
            return nullptr;
        }

        Object& object = std::get<Object>(data_);
        for (auto& [entry_key, entry_value] : object) {
            if (entry_key == key) {
                return &entry_value;
            }
        }

        return nullptr;
    }

    Value* EnsureObjectProperty(const std::string& key) {
        if (!IsObject()) {
            return nullptr;
        }

        Object& object = std::get<Object>(data_);
        for (auto& [entry_key, entry_value] : object) {
            if (entry_key == key) {
                return &entry_value;
            }
        }

        object.push_back({key, Value(0.0)});
        return &object.back().second;
    }

    double AsNumber(bool* out_ok = nullptr) const {
        if (std::holds_alternative<long long>(data_)) {
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return static_cast<double>(std::get<long long>(data_));
        }

        if (std::holds_alternative<double>(data_)) {
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return std::get<double>(data_);
        }

        if (std::holds_alternative<bool>(data_)) {
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return std::get<bool>(data_) ? 1.0 : 0.0;
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
        if (std::holds_alternative<long long>(data_)) {
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return std::get<long long>(data_);
        }

        if (std::holds_alternative<double>(data_)) {
            const double value = std::get<double>(data_);
            constexpr double kLongMin = static_cast<double>(std::numeric_limits<long long>::min());
            constexpr double kLongUpperExclusive = 9223372036854775808.0;  // 2^63
            if (!std::isfinite(value) ||
                std::trunc(value) != value ||
                value < kLongMin ||
                value >= kLongUpperExclusive) {
                if (out_ok != nullptr) {
                    *out_ok = false;
                }
                return 0;
            }

            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return static_cast<long long>(value);
        }

        if (std::holds_alternative<std::string>(data_)) {
            const std::string& text = std::get<std::string>(data_);
            long long parsed = 0;
            const char* begin = text.data();
            const char* end = begin + text.size();
            const auto parse_result = std::from_chars(begin, end, parsed);
            if (parse_result.ec == std::errc() && parse_result.ptr == end) {
                if (out_ok != nullptr) {
                    *out_ok = true;
                }
                return parsed;
            }

            if (out_ok != nullptr) {
                *out_ok = false;
            }
            return 0;
        }

        if (std::holds_alternative<bool>(data_)) {
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return std::get<bool>(data_) ? 1 : 0;
        }

        if (out_ok != nullptr) {
            *out_ok = false;
        }
        return 0;
    }

    bool AsBool() const {
        if (std::holds_alternative<bool>(data_)) {
            return std::get<bool>(data_);
        }

        if (std::holds_alternative<long long>(data_)) {
            return std::get<long long>(data_) != 0;
        }

        if (std::holds_alternative<double>(data_)) {
            return std::get<double>(data_) != 0.0;
        }

        if (std::holds_alternative<std::string>(data_)) {
            return !std::get<std::string>(data_).empty();
        }

        if (std::holds_alternative<List>(data_)) {
            return !std::get<List>(data_).empty();
        }

        return !std::get<Object>(data_).empty();
    }

    std::string ToString() const {
        return ToStringInternal(false);
    }

private:
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

    static std::string EscapeString(const std::string& text) {
        std::string escaped;
        escaped.reserve(text.size());

        for (char character : text) {
            if (character == '\\') {
                escaped += "\\\\";
            } else if (character == '"') {
                escaped += "\\\"";
            } else {
                escaped.push_back(character);
            }
        }

        return escaped;
    }

    std::string ToStringInternal(bool quote_string) const {
        if (std::holds_alternative<std::string>(data_)) {
            const std::string& raw = std::get<std::string>(data_);
            if (!quote_string) {
                return raw;
            }
            return "\"" + EscapeString(raw) + "\"";
        }

        if (std::holds_alternative<bool>(data_)) {
            return std::get<bool>(data_) ? "true" : "false";
        }

        if (std::holds_alternative<long long>(data_)) {
            return std::to_string(std::get<long long>(data_));
        }

        if (std::holds_alternative<double>(data_)) {
            return FormatNumber(std::get<double>(data_));
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

    std::variant<long long, double, std::string, bool, List, Object> data_;
};

struct VariableSlot {
    Value value;
    VariableKind kind = VariableKind::Dynamic;
};

}  // namespace clot::runtime

#endif  // CLOT_RUNTIME_VALUE_HPP
