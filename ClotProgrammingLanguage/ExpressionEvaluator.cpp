/*
 * Clot Programming Language
 * Copyright (c) 2024 Julian Clot Cordoba
 *
 * Licensed under the MIT License. You may obtain a copy of the License at:
 * https://opensource.org/licenses/MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ExpressionEvaluator.h"

namespace Clot {

    double ExpressionEvaluator::evaluate(const Tokens& tokens) {
        std::stack<double> values;
        std::stack<TokenType> ops;

        for (const auto& token : tokens) {
            if (token.type == TokenType::Number) {
                values.push(std::stod(token.value));
            }
            else if (token.type == TokenType::Identifier) {
                if (DOUBLE.count(token.value)) {
                    values.push(DOUBLE[token.value]);
                }
                else {
                    throw std::runtime_error("Variable no definida: " + token.value);
                }
            }
            else if (isOperator(token.type)) {
                while (!ops.empty() && precedence(ops.top()) >= precedence(token.type)) {
                    double val2 = values.top(); values.pop();
                    double val1 = values.top(); values.pop();
                    TokenType op = ops.top(); ops.pop();
                    values.push(applyOperator(val1, val2, op));
                }
                ops.push(token.type);
            }
        }

        while (!ops.empty()) {
            double val2 = values.top(); values.pop();
            double val1 = values.top(); values.pop();
            TokenType op = ops.top(); ops.pop();
            values.push(applyOperator(val1, val2, op));
        }

        return values.top();
    }

    bool ExpressionEvaluator::isOperator(TokenType type) {
        return type == TokenType::Plus || type == TokenType::Minus || type == TokenType::Multiply ||
            type == TokenType::Divide || type == TokenType::Modulo || type == TokenType::And ||
            type == TokenType::Or || type == TokenType::Not || type == TokenType::Equal ||
            type == TokenType::NotEqual || type == TokenType::Less || type == TokenType::LessEqual ||
            type == TokenType::Greater || type == TokenType::GreaterEqual;
    }

    int ExpressionEvaluator::precedence(TokenType type) {
        if (type == TokenType::Multiply || type == TokenType::Divide || type == TokenType::Modulo) return 3;
        if (type == TokenType::Plus || type == TokenType::Minus) return 2;
        if (type == TokenType::Equal || type == TokenType::NotEqual || type == TokenType::Less || type == TokenType::LessEqual || type == TokenType::Greater || type == TokenType::GreaterEqual) return 1;
        if (type == TokenType::And) return 0;
        if (type == TokenType::Or) return -1;
        return -2;
    }

    double ExpressionEvaluator::applyOperator(double val1, double val2, TokenType op) {
        switch (op) {
        case TokenType::Plus: return val1 + val2;
        case TokenType::Minus: return val1 - val2;
        case TokenType::Multiply: return val1 * val2;
        case TokenType::Divide: return val1 / val2;
        case TokenType::Modulo: return std::fmod(val1, val2);
        case TokenType::And: return val1 && val2;
        case TokenType::Or: return val1 || val2;
        case TokenType::Equal: return val1 == val2;
        case TokenType::NotEqual: return val1 != val2;
        case TokenType::Less: return val1 < val2;
        case TokenType::LessEqual: return val1 <= val2;
        case TokenType::Greater: return val1 > val2;
        case TokenType::GreaterEqual: return val1 >= val2;
		case TokenType::PlusEqual: return val1 += val2;
		case TokenType::MinusEqual: return val1 -= val2;
        default: throw std::runtime_error("Operador no soportado.");
        }
    }

} // namespace Clot
