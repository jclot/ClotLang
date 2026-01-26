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

#include "FunctionDeclaration.h"
#include <cctype>

namespace Clot {

    void FunctionDeclaration::declare(const Tokens& tokens, std::ifstream& file) {
        if (tokens.size() < 4 || tokens[0].type != TokenType::Func || tokens[2].type != TokenType::LeftParen) {
            throw std::runtime_error("Error en la declaración de función: formato inválido.");
        }

        FunctionName functionName = tokens[1].value;
        std::vector<VariableName> parameters;
		std::vector<bool> isReference;
        
        size_t i = 3;
        bool ref = false;
        int canRef = 0; 
        for (; i < tokens.size(); ++i) {
            if (tokens[i].type == TokenType::RightParen) {
                ++i;
                break;
            }
            if (tokens[i].type == TokenType::Comma) {
                continue;
            }
            if (tokens[i].type == TokenType::Ampersand) {
                ref = true;
                canRef += 1; 
                continue;
            }

			parameters.push_back(tokens[i].value);
			isReference.push_back(ref);
            ref = false;
        }

        if (i >= tokens.size() || tokens[i].type != TokenType::Colon) {
            throw std::runtime_error("Error en la declaración de función: falta el delimitador ':'");
        }

        std::vector<Line> body;
        Line line;
        bool endfuncFound = false;

        while (std::getline(file, line)) {
            // Trim left and right whitespace for comparison and storage
            std::string trimmed = line;
            // remove leading spaces/tabs
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            // remove trailing whitespace
            while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t' || trimmed.back() == '\r' || trimmed.back() == '\n')) trimmed.pop_back();

            if (trimmed == "endfunc") {
                endfuncFound = true;
                break;
            }

            // Require that function body lines are indented (start with whitespace) or empty
            if (!line.empty() && !std::isspace(static_cast<unsigned char>(line[0]))) {
                throw std::runtime_error("Error en la declaración de función: 'endfunc'");
            }

            // store trimmed line (without leading indentation) so execution uses consistent tokens
            body.push_back(trimmed);
        }

        if (!endfuncFound) {
            throw std::runtime_error("Error en la declaración de función: falta 'endfunc'");
        }

        functions[functionName] = { parameters, body, isReference };
        std::cout << "Función '" << functionName << "' declarada con " << parameters.size() << " parámetro(s)" << " y con " << canRef << " parametros de referencia." << std::endl;
    }

} // namespace Clot
