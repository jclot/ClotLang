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

namespace Clot {

    void PrintStatement::print(const Tokens& tokens) {
        if (tokens.size() < 5 || tokens[1].type != TokenType::LeftParen ||
            tokens.back().type != TokenType::SemiColon || tokens[tokens.size() - 2].type != TokenType::RightParen) {
            throw std::runtime_error("Error en print: formato invalido. Falta ';' al final.");
        }

        std::string message;
        for (size_t i = 2; i < tokens.size() - 2; ++i) {
            if (tokens[i].type == TokenType::Plus) {
                continue;
            }
            if (tokens[i].type == TokenType::Identifier && DOUBLE.count(tokens[i].value)) {
                message += std::to_string(DOUBLE[tokens[i].value]);
            }
            else {
                message += tokens[i].value;
            }
        }

        message.erase(std::remove(message.begin(), message.end(), '\"'), message.end());
        std::cout << "Funcion 'Print': " << message << std::endl;
    }

} // namespace Clot
