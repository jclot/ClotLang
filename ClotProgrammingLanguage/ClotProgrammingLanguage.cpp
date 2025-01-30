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

#include "Clot.h"
#include "Tokenizer.h"
#include "ExpressionEvaluator.h"
#include "VariableAssignment.h"
#include "PrintStatement.h"
#include "FunctionDeclaration.h"
#include "FunctionExecution.h"
#include "ModuleImporter.h"

namespace Clot {

    std::map<VariableName, double> DOUBLE;
    std::map<VariableName, std::string> STRING;
    std::map<FunctionName, Function> functions;

    void Interpret(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("No se pudo abrir el archivo: " + filename);
        }

        Line line;
        while (std::getline(file, line)) {
            Tokens tokens = Tokenizer::tokenize(line);
            if (tokens.empty() || tokens[0].type == TokenType::Comment) continue;

            try {
                if (tokens.size() > 1 && tokens[1].type == TokenType::Assignment) {
                    VariableAssignment::assign(tokens);
                }
                else if (tokens[0].type == TokenType::Print) {
                    PrintStatement::print(tokens);
                }
                else if (tokens[0].type == TokenType::Func) {
                    FunctionDeclaration::declare(tokens, file);
                }
                else if (tokens[0].type == TokenType::EndFunc) {
                    // Ignorar endfunc en el nivel superior
                    continue;
                }
                else if (functions.count(tokens[0].value)) {
                    FunctionExecution::execute(tokens);
                }
                else if (tokens[0].type == TokenType::Import) {
                    ModuleImporter::import(tokens);
                }
                else {
                    throw std::runtime_error("Instrucci�n no reconocida: " + line);
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }

} // namespace Clot

int main() {
    try {
        Clot::Interpret("test.clot");
    }
    catch (const std::exception& e) {
        std::cerr << "Error fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}