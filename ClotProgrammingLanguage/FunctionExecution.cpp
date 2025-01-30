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

namespace Clot {

    void FunctionExecution::execute(const Tokens& tokens) {
        if (tokens.empty()) {
            throw std::runtime_error("Error: nombre de función vacío.");
        }

        const FunctionName& functionName = tokens[0].value;
        if (!functions.count(functionName)) {
            throw std::runtime_error("Función no encontrada: " + functionName);
        }

        Function& function = functions[functionName];
        std::vector<double> arguments;

        for (size_t i = 2; i < tokens.size(); ++i) {
            if (tokens[i].type == TokenType::Comma || tokens[i].type == TokenType::LeftParen || tokens[i].type == TokenType::RightParen) continue;

            try {
                if (tokens[i].type == TokenType::Identifier && DOUBLE.count(tokens[i].value)) {
                    arguments.push_back(DOUBLE[tokens[i].value]);
                }
                else {
                    arguments.push_back(std::stod(tokens[i].value));
                }
            }
            catch (const std::exception& e) {
                throw std::runtime_error("Error al convertir argumento '" + tokens[i].value + "': " + e.what());
            }
        }

        if (arguments.size() != function.parameters.size()) {
            throw std::runtime_error("Número incorrecto de argumentos para la función " + functionName +
                ". Esperados: " + std::to_string(function.parameters.size()) +
                ", Recibidos: " + std::to_string(arguments.size()));
        }

        std::map<VariableName, double> previousContext = DOUBLE;

        for (size_t i = 0; i < function.parameters.size(); ++i) {
            DOUBLE[function.parameters[i]] = arguments[i];
        }

        for (const Line& line : function.body) {
            Tokens lineTokens = Tokenizer::tokenize(line);
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
            else {
                double result = ExpressionEvaluator::evaluate(lineTokens);
                std::cout << "Resultado de la expresión: " << result << std::endl;
            }
        }

        DOUBLE = previousContext;
    }

} // namespace Clot