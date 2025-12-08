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

#include "FunctionExecution.h"
#include "Tokenizer.h"
#include "VariableAssignment.h"
#include "PrintStatement.h"
#include "ExpressionEvaluator.h"
#include "ConditionalStatement.h"

namespace Clot {

    void FunctionExecution::execute(const Tokens& tokens) {
        if (tokens.empty()) {
            throw std::runtime_error("Error: nombre de función vacío.");
        }

		if (tokens.back().type != TokenType::SemiColon) {
			throw std::runtime_error("Error: falta ';' al final de la linea de la funcion.");
		}

        const FunctionName& functionName = tokens[0].value;
        if (!functions.count(functionName)) {
            throw std::runtime_error("Función no encontrada: " + functionName);
        }

        Function& function = functions[functionName];
        
        std::vector<Tokens> argsTokens;
        Tokens currentArg;

        for (size_t i = 2; i < tokens.size(); ++i) {
            if (tokens[i].type == TokenType::RightParen) {
                if (!currentArg.empty()) {
                    argsTokens.push_back(currentArg);
                }
                break;
            }
            if (tokens[i].type == TokenType::Comma) {
                if (!currentArg.empty()) {
                    argsTokens.push_back(currentArg);
                    currentArg.clear();
                }
                continue;
            }
			currentArg.push_back(tokens[i]);
        }

        if (argsTokens.size() != function.parameters.size()) {
            throw std::runtime_error("Número incorrecto de argumentos para la función " + functionName +
                ". Esperados: " + std::to_string(function.parameters.size()) +
                ", Recibidos: " + std::to_string(argsTokens.size()));
        }

        std::map<VariableName, double> previousContext = DOUBLE;
        std::vector<std::pair<VariableName, VariableName>> refMapping;

        for (size_t i = 0; i < function.parameters.size(); ++i) {
            if (function.isReference[i]) {
                // El argumento debe ser un identificador unico
                if (argsTokens[i].size() != 1 || argsTokens[i][0].type != TokenType::Identifier) {
                    throw std::runtime_error("El parámetro de referencia '" + function.parameters[i] + "' requiere un identificador.");
                }
                VariableName callerVar = argsTokens[i][0].value;
                if (!DOUBLE.count(callerVar)) {
                    throw std::runtime_error("Variable no encontrada en el contexto actual: " + callerVar);
                }
                // Inicialmente se copia el valor de la variable del llamador al parametro local
                DOUBLE[function.parameters[i]] = DOUBLE[callerVar];
                refMapping.push_back({ callerVar, function.parameters[i] });
            }
            else {
                double value = ExpressionEvaluator::evaluate(argsTokens[i]);
                DOUBLE[function.parameters[i]] = value;
            }
        }

        for (size_t i = 0; i < function.body.size(); ++i) {
            Tokens lineTokens = Tokenizer::tokenize(function.body[i]);
            if (lineTokens.empty()) continue;

            if (lineTokens.size() > 1 && lineTokens[1].type == TokenType::Assignment) {
                VariableAssignment::assign(lineTokens);
            }
            else if (lineTokens[0].type == TokenType::Print) {
                PrintStatement::print(lineTokens);
            }
            else if (functions.count(lineTokens[0].value)) {
                execute(lineTokens);
            }
            else if(lineTokens[0].type == TokenType::If) {
				i = ConditionalStatement::execute(function.body, i, lineTokens);
			}
            else {
                double result = ExpressionEvaluator::evaluate(lineTokens);
                std::cout << "Resultado de la expresión: " << result << std::endl;
            }
        }

        for (auto& mapping : refMapping) {
			previousContext[mapping.first] = DOUBLE[mapping.second];
        }

        DOUBLE = previousContext;
    }

} // namespace Clot