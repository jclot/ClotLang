#ifndef CLOT_RUNTIME_BIGINT_HPP
#define CLOT_RUNTIME_BIGINT_HPP

#include <algorithm>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace clot::runtime {

class BigInt {
public:
    BigInt() = default;

    BigInt(long long value) {
        if (value < 0) {
            negative_ = true;
            // LLONG_MIN cannot be negated directly in signed domain.
            if (value == std::numeric_limits<long long>::min()) {
                digits_ = "9223372036854775808";
            } else {
                digits_ = std::to_string(-value);
            }
        } else {
            digits_ = std::to_string(value);
        }
        Normalize();
    }

    static bool TryParse(const std::string& text, BigInt* out_integer) {
        if (out_integer == nullptr || text.empty()) {
            return false;
        }

        std::size_t index = 0;
        bool negative = false;
        if (text[index] == '+' || text[index] == '-') {
            negative = text[index] == '-';
            ++index;
        }
        if (index >= text.size()) {
            return false;
        }

        for (std::size_t i = index; i < text.size(); ++i) {
            if (text[i] < '0' || text[i] > '9') {
                return false;
            }
        }

        out_integer->negative_ = negative;
        out_integer->digits_ = text.substr(index);
        out_integer->Normalize();
        return true;
    }

    std::string ToString() const {
        if (negative_ && digits_ != "0") {
            return "-" + digits_;
        }
        return digits_;
    }

    bool IsZero() const {
        return digits_ == "0";
    }

    bool IsNegative() const {
        return negative_ && !IsZero();
    }

    bool ToLongLong(long long* out_value) const {
        if (out_value == nullptr) {
            return false;
        }

        unsigned long long magnitude = 0;
        if (!ToUnsignedLongLongMagnitude(&magnitude)) {
            return false;
        }

        if (negative_) {
            const unsigned long long kMaxAbsNegative = 9223372036854775808ULL;
            if (magnitude > kMaxAbsNegative) {
                return false;
            }
            if (magnitude == kMaxAbsNegative) {
                *out_value = std::numeric_limits<long long>::min();
            } else {
                *out_value = -static_cast<long long>(magnitude);
            }
            return true;
        }

        if (magnitude > static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
            return false;
        }
        *out_value = static_cast<long long>(magnitude);
        return true;
    }

    bool ToUnsignedLongLong(unsigned long long* out_value) const {
        if (out_value == nullptr || negative_) {
            return false;
        }
        return ToUnsignedLongLongMagnitude(out_value);
    }

    bool ToSizeT(std::size_t* out_value) const {
        if (out_value == nullptr || negative_) {
            return false;
        }

        unsigned long long magnitude = 0;
        if (!ToUnsignedLongLongMagnitude(&magnitude)) {
            return false;
        }
        if (magnitude > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
            return false;
        }
        *out_value = static_cast<std::size_t>(magnitude);
        return true;
    }

    double ToDouble(bool* out_ok = nullptr) const {
        std::string text = ToString();
        try {
            const double value = std::stod(text);
            if (out_ok != nullptr) {
                *out_ok = true;
            }
            return value;
        } catch (...) {
            if (out_ok != nullptr) {
                *out_ok = false;
            }
            return 0.0;
        }
    }

    template <typename T>
    T convert_to() const {
        if constexpr (std::is_same_v<T, std::string>) {
            return ToString();
        } else if constexpr (std::is_same_v<T, double>) {
            bool ok = false;
            const double value = ToDouble(&ok);
            if (!ok) {
                throw std::overflow_error("BigInt out of range for double");
            }
            return value;
        } else if constexpr (std::is_same_v<T, long long>) {
            long long value = 0;
            if (!ToLongLong(&value)) {
                throw std::overflow_error("BigInt out of range for long long");
            }
            return value;
        } else if constexpr (std::is_same_v<T, unsigned long long>) {
            unsigned long long value = 0;
            if (!ToUnsignedLongLong(&value)) {
                throw std::overflow_error("BigInt out of range for unsigned long long");
            }
            return value;
        } else if constexpr (std::is_same_v<T, unsigned>) {
            unsigned long long value = 0;
            if (!ToUnsignedLongLong(&value) || value > static_cast<unsigned long long>(std::numeric_limits<unsigned>::max())) {
                throw std::overflow_error("BigInt out of range for unsigned");
            }
            return static_cast<unsigned>(value);
        } else if constexpr (std::is_same_v<T, std::size_t>) {
            std::size_t value = 0;
            if (!ToSizeT(&value)) {
                throw std::overflow_error("BigInt out of range for size_t");
            }
            return value;
        } else {
            static_assert(!sizeof(T*), "Unsupported BigInt::convert_to type.");
        }
    }

    BigInt operator-() const {
        BigInt copy = *this;
        if (!copy.IsZero()) {
            copy.negative_ = !copy.negative_;
        }
        return copy;
    }

    BigInt& operator+=(const BigInt& rhs) {
        *this = *this + rhs;
        return *this;
    }

    BigInt& operator-=(const BigInt& rhs) {
        *this = *this - rhs;
        return *this;
    }

    BigInt& operator*=(const BigInt& rhs) {
        *this = *this * rhs;
        return *this;
    }

    BigInt& operator/=(const BigInt& rhs) {
        *this = *this / rhs;
        return *this;
    }

    BigInt& operator%=(const BigInt& rhs) {
        *this = *this % rhs;
        return *this;
    }

    friend bool operator==(const BigInt& lhs, const BigInt& rhs) {
        return lhs.negative_ == rhs.negative_ && lhs.digits_ == rhs.digits_;
    }

    friend bool operator!=(const BigInt& lhs, const BigInt& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const BigInt& lhs, const BigInt& rhs) {
        if (lhs.negative_ != rhs.negative_) {
            return lhs.negative_;
        }

        const int abs_cmp = CompareAbs(lhs.digits_, rhs.digits_);
        if (!lhs.negative_) {
            return abs_cmp < 0;
        }
        return abs_cmp > 0;
    }

    friend bool operator<=(const BigInt& lhs, const BigInt& rhs) {
        return lhs < rhs || lhs == rhs;
    }

    friend bool operator>(const BigInt& lhs, const BigInt& rhs) {
        return rhs < lhs;
    }

    friend bool operator>=(const BigInt& lhs, const BigInt& rhs) {
        return !(lhs < rhs);
    }

    friend BigInt operator+(const BigInt& lhs, const BigInt& rhs) {
        BigInt result;
        if (lhs.negative_ == rhs.negative_) {
            result.negative_ = lhs.negative_;
            result.digits_ = AddAbs(lhs.digits_, rhs.digits_);
            result.Normalize();
            return result;
        }

        const int cmp = CompareAbs(lhs.digits_, rhs.digits_);
        if (cmp == 0) {
            return BigInt(0LL);
        }
        if (cmp > 0) {
            result.negative_ = lhs.negative_;
            result.digits_ = SubAbs(lhs.digits_, rhs.digits_);
        } else {
            result.negative_ = rhs.negative_;
            result.digits_ = SubAbs(rhs.digits_, lhs.digits_);
        }
        result.Normalize();
        return result;
    }

    friend BigInt operator-(const BigInt& lhs, const BigInt& rhs) {
        return lhs + (-rhs);
    }

    friend BigInt operator*(const BigInt& lhs, const BigInt& rhs) {
        BigInt result;
        result.negative_ = lhs.negative_ != rhs.negative_;
        result.digits_ = MulAbs(lhs.digits_, rhs.digits_);
        result.Normalize();
        return result;
    }

    friend BigInt operator/(const BigInt& lhs, const BigInt& rhs) {
        BigInt quotient;
        BigInt remainder;
        DivMod(lhs, rhs, &quotient, &remainder);
        return quotient;
    }

    friend BigInt operator%(const BigInt& lhs, const BigInt& rhs) {
        BigInt quotient;
        BigInt remainder;
        DivMod(lhs, rhs, &quotient, &remainder);
        return remainder;
    }

private:
    static int CompareAbs(const std::string& lhs, const std::string& rhs) {
        if (lhs.size() < rhs.size()) {
            return -1;
        }
        if (lhs.size() > rhs.size()) {
            return 1;
        }
        if (lhs < rhs) {
            return -1;
        }
        if (lhs > rhs) {
            return 1;
        }
        return 0;
    }

    static std::string AddAbs(const std::string& lhs, const std::string& rhs) {
        std::string result;
        int carry = 0;
        int i = static_cast<int>(lhs.size()) - 1;
        int j = static_cast<int>(rhs.size()) - 1;
        while (i >= 0 || j >= 0 || carry > 0) {
            int sum = carry;
            if (i >= 0) {
                sum += lhs[static_cast<std::size_t>(i--)] - '0';
            }
            if (j >= 0) {
                sum += rhs[static_cast<std::size_t>(j--)] - '0';
            }
            result.push_back(static_cast<char>('0' + (sum % 10)));
            carry = sum / 10;
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    static std::string SubAbs(const std::string& lhs, const std::string& rhs) {
        // Requires lhs >= rhs in absolute value.
        std::string result;
        int borrow = 0;
        int i = static_cast<int>(lhs.size()) - 1;
        int j = static_cast<int>(rhs.size()) - 1;
        while (i >= 0) {
            int digit = (lhs[static_cast<std::size_t>(i)] - '0') - borrow;
            if (j >= 0) {
                digit -= rhs[static_cast<std::size_t>(j)] - '0';
            }

            if (digit < 0) {
                digit += 10;
                borrow = 1;
            } else {
                borrow = 0;
            }

            result.push_back(static_cast<char>('0' + digit));
            --i;
            --j;
        }

        while (result.size() > 1 && result.back() == '0') {
            result.pop_back();
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    static std::string MulAbs(const std::string& lhs, const std::string& rhs) {
        if (lhs == "0" || rhs == "0") {
            return "0";
        }

        std::vector<int> digits(lhs.size() + rhs.size(), 0);
        for (int i = static_cast<int>(lhs.size()) - 1; i >= 0; --i) {
            for (int j = static_cast<int>(rhs.size()) - 1; j >= 0; --j) {
                const int product = (lhs[static_cast<std::size_t>(i)] - '0') * (rhs[static_cast<std::size_t>(j)] - '0');
                const std::size_t pos_low = static_cast<std::size_t>(i + j + 1);
                const std::size_t pos_high = static_cast<std::size_t>(i + j);
                const int total = digits[pos_low] + product;
                digits[pos_low] = total % 10;
                digits[pos_high] += total / 10;
            }
        }

        std::string result;
        result.reserve(digits.size());
        bool started = false;
        for (int digit : digits) {
            if (digit == 0 && !started) {
                continue;
            }
            started = true;
            result.push_back(static_cast<char>('0' + digit));
        }
        return result.empty() ? "0" : result;
    }

    static BigInt Abs(const BigInt& value) {
        BigInt copy = value;
        copy.negative_ = false;
        return copy;
    }

    static void DivMod(const BigInt& lhs, const BigInt& rhs, BigInt* out_quotient, BigInt* out_remainder) {
        if (out_quotient == nullptr || out_remainder == nullptr) {
            return;
        }

        if (rhs.IsZero()) {
            *out_quotient = BigInt(0LL);
            *out_remainder = BigInt(0LL);
            return;
        }

        BigInt dividend = Abs(lhs);
        BigInt divisor = Abs(rhs);
        if (CompareAbs(dividend.digits_, divisor.digits_) < 0) {
            *out_quotient = BigInt(0LL);
            *out_remainder = dividend;
            out_remainder->negative_ = lhs.negative_ && !out_remainder->IsZero();
            return;
        }

        std::string quotient_digits;
        quotient_digits.reserve(dividend.digits_.size());
        BigInt remainder(0LL);

        for (char ch : dividend.digits_) {
            if (!remainder.IsZero()) {
                remainder.digits_.push_back(ch);
            } else {
                remainder.digits_ = std::string(1, ch);
            }
            remainder.Normalize();

            int quotient_digit = 0;
            while (CompareAbs(remainder.digits_, divisor.digits_) >= 0) {
                remainder.digits_ = SubAbs(remainder.digits_, divisor.digits_);
                remainder.Normalize();
                ++quotient_digit;
            }
            quotient_digits.push_back(static_cast<char>('0' + quotient_digit));
        }

        BigInt quotient;
        quotient.negative_ = lhs.negative_ != rhs.negative_;
        quotient.digits_ = quotient_digits;
        quotient.Normalize();

        remainder.negative_ = lhs.negative_ && !remainder.IsZero();
        remainder.Normalize();

        *out_quotient = std::move(quotient);
        *out_remainder = std::move(remainder);
    }

    bool ToUnsignedLongLongMagnitude(unsigned long long* out_value) const {
        if (out_value == nullptr) {
            return false;
        }

        unsigned long long value = 0;
        for (char digit : digits_) {
            const unsigned d = static_cast<unsigned>(digit - '0');
            if (value > (std::numeric_limits<unsigned long long>::max() - d) / 10ULL) {
                return false;
            }
            value = value * 10ULL + d;
        }
        *out_value = value;
        return true;
    }

    void Normalize() {
        std::size_t first_non_zero = digits_.find_first_not_of('0');
        if (first_non_zero == std::string::npos) {
            digits_ = "0";
            negative_ = false;
            return;
        }
        if (first_non_zero > 0) {
            digits_.erase(0, first_non_zero);
        }
        if (digits_ == "0") {
            negative_ = false;
        }
    }

    bool negative_ = false;
    std::string digits_ = "0";
};

}  // namespace clot::runtime

#endif  // CLOT_RUNTIME_BIGINT_HPP
