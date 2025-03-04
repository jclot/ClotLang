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

namespace Clot {

    void VariableAssignment::assign(const Tokens& tokens) {
        if (tokens.size() < 4 || tokens.back().type != TokenType::SemiColon) {
            throw std::runtime_error("Error en asignación: formato inválido o falta ';'.");
        }

        const VariableName& variableName = tokens[0].value;
        TokenType opType = tokens[1].type;
        Tokens expressionTokens(tokens.begin() + 2, tokens.end() - 1);

        // Asignación simple: x = expr
        if (opType == TokenType::Assignment) {
            double value = ExpressionEvaluator::evaluate(expressionTokens);
            DOUBLE[variableName] = value;
            std::cout << "Variable '" << variableName << "' asignada con valor: " << value << std::endl;
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