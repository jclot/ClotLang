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

		for (size_t i = 0; i < tokens.size(); ++i) {
			const auto& token = tokens[i];
			if (token.type == TokenType::Number) {
				values.push(std::stod(token.value));
			}
			else if (token.type == TokenType::Boolean) {
				values.push(token.value == "true" ? 1.0 : 0.0);
			}
			else if (token.type == TokenType::Identifier) {
				if (DOUBLE.count(token.value)) {
					values.push(DOUBLE[token.value]);
				}
				else if (INT.count(token.value)) {
					values.push(static_cast<double>(INT[token.value]));
				}
				else if (LONG.count(token.value)) {
					values.push(static_cast<double>(LONG[token.value]));
				}
				else if (BYTE.count(token.value)) {
					values.push(static_cast<double>(BYTE[token.value]));
				}
				else if (BOOL.count(token.value)) {
					values.push(BOOL[token.value] ? 1.0 : 0.0);
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
			type == TokenType::Divide || type == TokenType::Modulo || type == TokenType::Power ||
			type == TokenType::And || type == TokenType::Or || type == TokenType::Not ||
			type == TokenType::Equal || type == TokenType::NotEqual || type == TokenType::Less ||
			type == TokenType::LessEqual || type == TokenType::Greater || type == TokenType::GreaterEqual;
	}

	int ExpressionEvaluator::precedence(TokenType type) {
		if (type == TokenType::Power) return 4;
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
		case TokenType::Power: return std::pow(val1, val2);
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

	std::string ExpressionEvaluator::evaluateStringExpression(const Tokens& tokens) {
		std::string result;
		bool expectOperand = true;
		for (size_t i = 0; i < tokens.size(); ++i) {
			const auto& token = tokens[i];
			if (expectOperand) {
				if (token.type == TokenType::String) {
					std::string str = token.value;
					str.erase(std::remove(str.begin(), str.end(), '"'), str.end());
					result += str;
				}
				else if (token.type == TokenType::Identifier) {
					if (STRING.count(token.value)) {
						result += STRING[token.value];
					}
					else if (DOUBLE.count(token.value)) {
						result += std::to_string(DOUBLE[token.value]);
					}
					else {
						throw std::runtime_error("Variable no definida: " + token.value);
					}
				}
				else if (token.type == TokenType::Number) {
					result += token.value;
				}
				else {
					throw std::runtime_error("Operando no válido en expresión de string.");
				}
				expectOperand = false;
			}
			else {
				if (token.type != TokenType::Plus) {
					throw std::runtime_error("Solo se permite concatenación con '+'.");
				}
				expectOperand = true;
			}
		}
		return result;
	}

}
