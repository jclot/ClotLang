#ifndef CLOT_RUNTIME_DECIMAL_HPP
#define CLOT_RUNTIME_DECIMAL_HPP

#include <algorithm>
#include <string>
#include <utility>

#include "clot/runtime/bigint.hpp"

namespace clot::runtime {

class Decimal {
public:
    Decimal() = default;

    Decimal(BigInt coefficient, int scale)
        : coefficient_(std::move(coefficient)),
          scale_(scale) {
        Normalize();
    }

    static bool TryParse(const std::string& text, Decimal* out_decimal) {
        if (out_decimal == nullptr || text.empty()) {
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

        std::string digits;
        digits.reserve(text.size());
        bool seen_dot = false;
        int scale = 0;
        bool has_digit = false;

        for (; index < text.size(); ++index) {
            const char character = text[index];
            if (character >= '0' && character <= '9') {
                digits.push_back(character);
                has_digit = true;
                if (seen_dot) {
                    ++scale;
                }
                continue;
            }
            if (character == '.' && !seen_dot) {
                seen_dot = true;
                continue;
            }
            return false;
        }

        if (!has_digit) {
            return false;
        }

        std::size_t first_non_zero = digits.find_first_not_of('0');
        if (first_non_zero == std::string::npos) {
            *out_decimal = Decimal(BigInt(0), 0);
            return true;
        }

        digits = digits.substr(first_non_zero);
        std::string signed_digits = negative ? "-" + digits : digits;

        BigInt parsed;
        if (!BigInt::TryParse(signed_digits, &parsed)) {
            return false;
        }

        *out_decimal = Decimal(std::move(parsed), scale);
        return true;
    }

    static Decimal FromBigInt(const BigInt& value) {
        return Decimal(value, 0);
    }

    std::string ToString() const {
        std::string coefficient_text = coefficient_.ToString();
        bool negative = false;
        if (!coefficient_text.empty() && coefficient_text[0] == '-') {
            negative = true;
            coefficient_text.erase(coefficient_text.begin());
        }

        if (scale_ == 0) {
            return negative ? "-" + coefficient_text : coefficient_text;
        }

        std::string rendered;
        if (static_cast<int>(coefficient_text.size()) <= scale_) {
            rendered = "0.";
            rendered.append(static_cast<std::size_t>(scale_ - static_cast<int>(coefficient_text.size())), '0');
            rendered += coefficient_text;
        } else {
            const std::size_t split = coefficient_text.size() - static_cast<std::size_t>(scale_);
            rendered = coefficient_text.substr(0, split);
            rendered.push_back('.');
            rendered += coefficient_text.substr(split);
        }

        return negative ? "-" + rendered : rendered;
    }

    bool ToDouble(double* out_number) const {
        if (out_number == nullptr) {
            return false;
        }
        try {
            *out_number = std::stod(ToString());
            return true;
        } catch (...) {
            return false;
        }
    }

    bool ToBigIntIfIntegral(BigInt* out_integer) const {
        if (out_integer == nullptr) {
            return false;
        }
        if (scale_ != 0) {
            return false;
        }
        *out_integer = coefficient_;
        return true;
    }

    const BigInt& Coefficient() const {
        return coefficient_;
    }

    int Scale() const {
        return scale_;
    }

    friend bool operator==(const Decimal& lhs, const Decimal& rhs) {
        BigInt left;
        BigInt right;
        int scale = 0;
        AlignScales(lhs, rhs, &left, &right, &scale);
        (void)scale;
        return left == right;
    }

    friend bool operator!=(const Decimal& lhs, const Decimal& rhs) {
        return !(lhs == rhs);
    }

    friend bool operator<(const Decimal& lhs, const Decimal& rhs) {
        BigInt left;
        BigInt right;
        int scale = 0;
        AlignScales(lhs, rhs, &left, &right, &scale);
        (void)scale;
        return left < right;
    }

    friend bool operator<=(const Decimal& lhs, const Decimal& rhs) {
        return lhs < rhs || lhs == rhs;
    }

    friend bool operator>(const Decimal& lhs, const Decimal& rhs) {
        return rhs < lhs;
    }

    friend bool operator>=(const Decimal& lhs, const Decimal& rhs) {
        return !(lhs < rhs);
    }

    friend Decimal operator+(const Decimal& lhs, const Decimal& rhs) {
        BigInt left;
        BigInt right;
        int scale = 0;
        AlignScales(lhs, rhs, &left, &right, &scale);
        return Decimal(left + right, scale);
    }

    friend Decimal operator-(const Decimal& lhs, const Decimal& rhs) {
        BigInt left;
        BigInt right;
        int scale = 0;
        AlignScales(lhs, rhs, &left, &right, &scale);
        return Decimal(left - right, scale);
    }

    friend Decimal operator*(const Decimal& lhs, const Decimal& rhs) {
        return Decimal(lhs.coefficient_ * rhs.coefficient_, lhs.scale_ + rhs.scale_);
    }

    static bool Divide(
        const Decimal& lhs,
        const Decimal& rhs,
        int precision_digits,
        Decimal* out_decimal,
        std::string* out_error) {
        if (out_decimal == nullptr) {
            if (out_error != nullptr) {
                *out_error = "Error interno: salida nula en division decimal.";
            }
            return false;
        }

        if (rhs.coefficient_ == 0) {
            if (out_error != nullptr) {
                *out_error = "Division por cero.";
            }
            return false;
        }

        if (precision_digits < 0) {
            precision_digits = 0;
        }

        const BigInt scaled_numerator = lhs.coefficient_ * Pow10(precision_digits);
        const BigInt quotient = scaled_numerator / rhs.coefficient_;
        int scale = lhs.scale_ - rhs.scale_ + precision_digits;
        if (scale < 0) {
            *out_decimal = Decimal(quotient * Pow10(-scale), 0);
            return true;
        }

        *out_decimal = Decimal(quotient, scale);
        return true;
    }

private:
    static BigInt Pow10(int exponent) {
        if (exponent <= 0) {
            return BigInt(1);
        }

        BigInt power = 1;
        for (int i = 0; i < exponent; ++i) {
            power *= 10;
        }
        return power;
    }

    static void AlignScales(
        const Decimal& lhs,
        const Decimal& rhs,
        BigInt* out_left,
        BigInt* out_right,
        int* out_scale) {
        const int target_scale = std::max(lhs.scale_, rhs.scale_);
        const int lhs_delta = target_scale - lhs.scale_;
        const int rhs_delta = target_scale - rhs.scale_;

        if (out_left != nullptr) {
            *out_left = lhs.coefficient_ * Pow10(lhs_delta);
        }
        if (out_right != nullptr) {
            *out_right = rhs.coefficient_ * Pow10(rhs_delta);
        }
        if (out_scale != nullptr) {
            *out_scale = target_scale;
        }
    }

    void Normalize() {
        if (coefficient_ == 0) {
            scale_ = 0;
            return;
        }

        if (scale_ < 0) {
            coefficient_ *= Pow10(-scale_);
            scale_ = 0;
            return;
        }

        const BigInt ten = 10;
        while (scale_ > 0) {
            BigInt remainder = coefficient_ % ten;
            if (remainder < 0) {
                remainder = -remainder;
            }
            if (remainder != 0) {
                break;
            }
            coefficient_ /= ten;
            --scale_;
        }
    }

    BigInt coefficient_ = 0;
    int scale_ = 0;
};

}  // namespace clot::runtime

#endif  // CLOT_RUNTIME_DECIMAL_HPP
