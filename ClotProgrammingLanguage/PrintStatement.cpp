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

#include "PrintStatement.h"
#include "ExpressionEvaluator.h"

namespace Clot {

	void PrintStatement::print(const Tokens& tokens) {
		if (tokens.size() < 5 || tokens[1].type != TokenType::LeftParen ||
			tokens.back().type != TokenType::SemiColon || tokens[tokens.size() - 2].type != TokenType::RightParen) {
			throw std::runtime_error("Error en print: formato invalido. Falta ';' al final.");
		}

		// Extract tokens between parentheses
		Tokens contentTokens(tokens.begin() + 2, tokens.end() - 2);

		// Si es un string literal
		if (contentTokens.size() == 1 && contentTokens[0].type == TokenType::String) {
			std::string message = contentTokens[0].value;
			message.erase(std::remove(message.begin(), message.end(), '"'), message.end());
			std::cout << "Funcion 'Print': " << message << std::endl;
		}
		// Si es un booleano literal
		else if (contentTokens.size() == 1 && contentTokens[0].type == TokenType::Boolean) {
			std::cout << "Funcion 'Print': " << (contentTokens[0].value == "true" ? "true" : "false") << std::endl;
		}
		// Si es una variable string, double, bool, lista u objeto
		else if (contentTokens.size() == 1 && contentTokens[0].type == TokenType::Identifier) {
			const std::string& varName = contentTokens[0].value;
			if (STRING.count(varName)) {
				std::cout << "Funcion 'Print': " << STRING[varName] << std::endl;
			}
			else if (DOUBLE.count(varName)) {
				std::cout << "Funcion 'Print': " << DOUBLE[varName] << std::endl;
			}
			else if (BOOL.count(varName)) {
				std::cout << "Funcion 'Print': " << (BOOL[varName] ? "true" : "false") << std::endl;
			}

			else if (LIST.count(varName)) {
				std::cout << "Funcion 'Print': [";
				for (size_t i = 0; i < LIST[varName].size(); ++i) {
					const auto& v = LIST[varName][i];
					if (std::holds_alternative<long>(v)) std::cout << std::get<long>(v);
					else if (std::holds_alternative<double>(v)) std::cout << std::get<double>(v);
					else if (std::holds_alternative<std::string>(v)) std::cout << '"' << std::get<std::string>(v) << '"';
					else if (std::holds_alternative<bool>(v)) std::cout << (std::get<bool>(v) ? "true" : "false");
					if (i + 1 < LIST[varName].size()) std::cout << ", ";
				}
				std::cout << "]" << std::endl;
			}
			else if (OBJECT.count(varName)) {
				std::cout << "Funcion 'Print': {";
				const auto& obj = OBJECT[varName];
				for (size_t j = 0; j < obj.size(); ++j) {
					const auto& key = obj[j].first;
					const auto& val = obj[j].second;
					std::cout << key << ": ";
					if (std::holds_alternative<double>(val)) std::cout << std::get<double>(val);
					else if (std::holds_alternative<std::string>(val)) std::cout << '"' << std::get<std::string>(val) << '"';
					else if (std::holds_alternative<bool>(val)) std::cout << (std::get<bool>(val) ? "true" : "false");
					else if (std::holds_alternative<List>(val)) {
						std::cout << "[";
						const auto& l = std::get<List>(val);
						for (size_t i = 0; i < l.size(); ++i) {
							if (std::holds_alternative<double>(l[i])) std::cout << std::get<double>(l[i]);
							else if (std::holds_alternative<std::string>(l[i])) std::cout << '"' << std::get<std::string>(l[i]) << '"';
							else if (std::holds_alternative<bool>(l[i])) std::cout << (std::get<bool>(l[i]) ? "true" : "false");
							if (i + 1 < l.size()) std::cout << ", ";
						}
						std::cout << "]";
					}
					if (j + 1 < obj.size()) std::cout << ", ";
				}
				std::cout << "}" << std::endl;
			}
			else if (INT.count(varName)) {
				std::cout << "Funcion 'Print': " << INT[varName] << std::endl;
			}
			else if (LONG.count(varName)) {
				std::cout << "Funcion 'Print': " << LONG[varName] << std::endl;
			}
			else if (BYTE.count(varName)) {
				std::cout << "Funcion 'Print': " << static_cast<int>(BYTE[varName]) << std::endl;
			}
			else {
				throw std::runtime_error("Variable no definida: " + varName);
			}
		}
		// Si contiene strings o variables string, usar el evaluador de strings
		else {
			bool hasString = false;
			for (const auto& t : contentTokens) {
				if (t.type == TokenType::String || (t.type == TokenType::Identifier && STRING.count(t.value))) {
					hasString = true;
					break;
				}
			}
			if (hasString) {
				std::string result = ExpressionEvaluator::evaluateStringExpression(contentTokens);
				std::cout << "Funcion 'Print': " << result << std::endl;
			}
			else {
				double result = ExpressionEvaluator::evaluate(contentTokens);
				std::cout << "Funcion 'Print': " << result << std::endl;
			}
		}
	}

} // namespace Clot
