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

#include "VariableAssignment.h"
#include "ExpressionEvaluator.h"
#include "Clot.h"

namespace Clot {

	void VariableAssignment::assign(const Tokens& tokens) {
		//  long/byte: long var = valor; byte var = valor;
		if (tokens.size() == 5 && tokens[0].type == TokenType::Long && tokens[2].type == TokenType::Assignment && tokens[3].type == TokenType::Number && tokens[4].type == TokenType::SemiColon) {
			const VariableName& variableName = tokens[1].value;
			long long lvalue = 0;
			try {
				lvalue = std::stoll(tokens[3].value);
			}
			catch (...) {
				throw std::runtime_error("Valor fuera de rango para long long");
			}
			LONG[variableName] = lvalue;
			std::cout << "Variable '" << variableName << "' asignada con valor long: " << lvalue << std::endl;
			return;
		}
		if (tokens.size() == 5 && tokens[0].type == TokenType::Byte && tokens[2].type == TokenType::Assignment && tokens[3].type == TokenType::Number && tokens[4].type == TokenType::SemiColon) {
			const VariableName& variableName = tokens[1].value;
			int byteVal = std::stoi(tokens[3].value);
			if (byteVal < 0 || byteVal >255) throw std::runtime_error("Valor fuera de rango para byte (0-255)");
			BYTE[variableName] = static_cast<unsigned char>(byteVal);
			std::cout << "Variable '" << variableName << "' asignada con valor byte: " << byteVal << std::endl;
			return;
		}

		// normal: x = expr
		if (tokens.size() < 4 || tokens.back().type != TokenType::SemiColon) {
			throw std::runtime_error("Error en asignación: formato inválido o falta ';'.");
		}

		const VariableName& variableName = tokens[0].value;
		TokenType opType = tokens[1].type;
		Tokens expressionTokens(tokens.begin() + 2, tokens.end() - 1);

		// simple: x = expr
		if (opType == TokenType::Assignment) {
			// Listas: [ ... ]
			if (expressionTokens.size() >= 3 && expressionTokens[0].type == TokenType::LeftBracket && expressionTokens.back().type == TokenType::RightBracket) {
				List listValue;
				for (size_t i = 1; i < expressionTokens.size() - 1; ++i) {
					const auto& t = expressionTokens[i];
					if (t.type == TokenType::Number) 
					{ 
						if(t.value.find('.') != std::string::npos)
							listValue.push_back(std::stod(t.value)); 
						else {
							listValue.push_back(std::stod(t.value));
						}
					}
					else if (t.type == TokenType::String) {
						std::string str = t.value;
						str.erase(std::remove(str.begin(), str.end(), '"'), str.end());
						listValue.push_back(str);
					}
					else if (t.type == TokenType::Boolean) listValue.push_back(t.value == "true");
					else if (t.type == TokenType::Identifier) {
						if (LONG.count(t.value)) listValue.push_back((long)LONG[t.value]);
						else if (INT.count(t.value)) listValue.push_back(INT[t.value]);
						else if (DOUBLE.count(t.value)) listValue.push_back(DOUBLE[t.value]);
						else if (BYTE.count(t.value)) listValue.push_back((double)BYTE[t.value]);
						else if (BOOL.count(t.value)) listValue.push_back(BOOL[t.value]);
						else if (STRING.count(t.value)) listValue.push_back(STRING[t.value]);
						else throw std::runtime_error("Variable no definida en lista: " + t.value);
					}
					else if (t.type == TokenType::Comma) continue;
				}
				LIST[variableName] = listValue;
				std::cout << "Variable '" << variableName << "' asignada con lista." << std::endl;
			}
			// Objetos: { ... }
			else if (expressionTokens.size() >= 3 && expressionTokens[0].type == TokenType::LeftBrace && expressionTokens.back().type == TokenType::RightBrace) {
				Object objValue;
				for (size_t i = 1; i < expressionTokens.size() - 1; ++i) {
					if (expressionTokens[i].type == TokenType::Identifier && i + 2 < expressionTokens.size() && expressionTokens[i + 1].type == TokenType::Colon) {
						std::string key = expressionTokens[i].value;
						const auto& valToken = expressionTokens[i + 2];
						if (valToken.type == TokenType::Number) objValue.push_back({ key, std::stod(valToken.value) });
						else if (valToken.type == TokenType::String) {
							std::string str = valToken.value;
							str.erase(std::remove(str.begin(), str.end(), '"'), str.end());
							objValue.push_back({ key, str });
						}
						else if (valToken.type == TokenType::Boolean) objValue.push_back({ key, valToken.value == "true" });
						i += 2;
					}
				}
				OBJECT[variableName] = objValue;
				std::cout << "Variable '" << variableName << "' asignada con objeto." << std::endl;
			}
			// Booleanos
			else if (expressionTokens.size() == 1 && expressionTokens[0].type == TokenType::Boolean) {
				BOOL[variableName] = (expressionTokens[0].value == "true");
				std::cout << "Variable '" << variableName << "' asignada con valor booleano: " << (BOOL[variableName] ? "true" : "false") << std::endl;
			}
			// Strings
			else {
				bool hasString = false;
				for (const auto& t : expressionTokens) {
					if (t.type == TokenType::String || (t.type == TokenType::Identifier && STRING.count(t.value))) {
						hasString = true;
						break;
					}
				}
				if (hasString) {
					std::string strValue = ExpressionEvaluator::evaluateStringExpression(expressionTokens);
					STRING[variableName] = strValue;
					std::cout << "Variable '" << variableName << "' asignada con valor string: " << strValue << std::endl;
				}
				else if (expressionTokens.size() == 1 && expressionTokens[0].type == TokenType::Number) {
					std::string valStr = expressionTokens[0].value;
					if (valStr.find('.') != std::string::npos) {
						DOUBLE[variableName] = std::stod(valStr);
						std::cout << "Variable '" << variableName << "' asignada con valor: " << DOUBLE[variableName] << std::endl;
					}
					else {
						INT[variableName] = std::stoi(valStr);
						std::cout << "Variable '" << variableName << "' asignada con valor: " << INT[variableName] << std::endl;
					}
				}
				else {
					double value = ExpressionEvaluator::evaluate(expressionTokens);
					DOUBLE[variableName] = value;
					std::cout << "Variable '" << variableName << "' asignada con valor: " << value << std::endl;
				}
			}
		}
		// Operador +=: x += expr
		else if (opType == TokenType::PlusEqual) {
			if (!DOUBLE.count(variableName)) {
				throw std::runtime_error("Variable no definida: " + variableName);
			}
			double addition = ExpressionEvaluator::evaluate(expressionTokens);
			DOUBLE[variableName] += addition;
			std::cout << "Variable '" << variableName << "' incrementada a: " << DOUBLE[variableName] << std::endl;
		}
		// Operador -=: x -= expr
		else if (opType == TokenType::MinusEqual) {
			if (!DOUBLE.count(variableName)) {
				throw std::runtime_error("Variable no definida: " + variableName);
			}
			double subtraction = ExpressionEvaluator::evaluate(expressionTokens);
			DOUBLE[variableName] -= subtraction;
			std::cout << "Variable '" << variableName << "' decrementada a: " << DOUBLE[variableName] << std::endl;
		}
		else {
			throw std::runtime_error("Operador de asignación no reconocido.");
		}
	}


} // namespace Clot