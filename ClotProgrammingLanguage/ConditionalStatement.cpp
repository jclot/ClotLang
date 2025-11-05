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
 
#include "ConditionalStatement.h"
#include "Tokenizer.h"
#include "VariableAssignment.h"
#include "PrintStatement.h"
#include "FunctionExecution.h"
#include "ModuleImporter.h"
#include "ExpressionEvaluator.h"
#include <stdexcept>


namespace Clot {

    namespace {

        bool isControlToken(const Tokens& tokens, TokenType type) {
            return !tokens.empty() && tokens[0].type == type;
        }

    } // anonymous namespace

    void ConditionalStatement::execute(std::ifstream& file, const Tokens& tokens) {
        if (tokens.empty() || tokens[0].type != TokenType::If) {
            throw std::runtime_error("Error en condicional: instruccin 'if' no vlida.");
        }
        if (tokens.back().type != TokenType::Colon) {
            throw std::runtime_error("Error en condicional: falta ':' al final de la condicin.");
        }

        Tokens conditionTokens(tokens.begin() + 1, tokens.end() - 1);

        std::vector<Line> ifLines;
        std::vector<Line> elseLines;
        bool parsingElse = false;
        int nestedLevel = 0;
        Line blockLine;

        bool endifFound = false;

        while (std::getline(file, blockLine)) {
            Tokens blockTokens = Tokenizer::tokenize(blockLine);

            if (isControlToken(blockTokens, TokenType::Else) && nestedLevel == 0) {
                if (blockTokens.back().type != TokenType::Colon) {
                    throw std::runtime_error("Error en condicional: falta ':' en 'else'.");
                }
                parsingElse = true;
                continue;
            }

            if (isControlToken(blockTokens, TokenType::EndIf)) {
                if (nestedLevel == 0) {
                    endifFound = true;
                    break;
                }
                else {
                    if (parsingElse) {
                        elseLines.push_back(blockLine);
                    }
                    else {
                        ifLines.push_back(blockLine);
                    }
                    --nestedLevel;
                    continue;
                }
            }

            if (isControlToken(blockTokens, TokenType::If)) {
                ++nestedLevel;
            }

            if (parsingElse) {
                elseLines.push_back(blockLine);
            }
            else {
                ifLines.push_back(blockLine);
            }
        }

        if (nestedLevel != 0 || !endifFound) {
            throw std::runtime_error("Error en condicional: falta 'endif'.");
        }

        bool conditionResult = evaluateCondition(conditionTokens);

        if (conditionResult) {
            executeBlock(ifLines);
        }
        else {
            executeBlock(elseLines);
        }
    }

    size_t ConditionalStatement::execute(const std::vector<Line>& body, size_t currentIndex, const Tokens& tokens) {
        if (tokens.empty() || tokens[0].type != TokenType::If) {
            throw std::runtime_error("Error en condicional: instruccin 'if' no vlida.");
        }
        if (tokens.back().type != TokenType::Colon) {
            throw std::runtime_error("Error en condicional: falta ':' al final de la condicin.");
        }

        Tokens conditionTokens(tokens.begin() + 1, tokens.end() - 1);

        std::vector<Line> ifLines;
        std::vector<Line> elseLines;
        bool parsingElse = false;
        int nestedLevel = 0;

        size_t index = currentIndex + 1;
        for (; index < body.size(); ++index) {
            Tokens lineTokens = Tokenizer::tokenize(body[index]);

            if (isControlToken(lineTokens, TokenType::Else) && nestedLevel == 0) {
                if (lineTokens.back().type != TokenType::Colon) {
                    throw std::runtime_error("Error en condicional: falta ':' en 'else'.");
                }
                parsingElse = true;
                continue;
            }

            if (isControlToken(lineTokens, TokenType::EndIf)) {
                if (nestedLevel == 0) {
                    break;
                }
                else {
                    if (parsingElse) {
                        elseLines.push_back(body[index]);
                    }
                    else {
                        ifLines.push_back(body[index]);
                    }
                    --nestedLevel;
                    continue;
                }
            }

            if (isControlToken(lineTokens, TokenType::If)) {
                ++nestedLevel;
            }

            if (parsingElse) {
                elseLines.push_back(body[index]);
            }
            else {
                ifLines.push_back(body[index]);
            }
        }

        if (index >= body.size()) {
            throw std::runtime_error("Error en condicional: falta 'endif'.");
        }

        bool conditionResult = evaluateCondition(conditionTokens);

        if (conditionResult) {
            executeBlock(ifLines);
        }
        else {
            executeBlock(elseLines);
        }

        return index;
    }

    bool ConditionalStatement::evaluateCondition(const Tokens& conditionTokens) {
        if (conditionTokens.empty()) {
            throw std::runtime_error("Error en condicional: condicin vaca.");
        }

        double result = ExpressionEvaluator::evaluate(conditionTokens);
        return result != 0.0;
    }

    void ConditionalStatement::executeBlock(const std::vector<Line>& lines) {
        for (size_t i = 0; i < lines.size(); ++i) {
            Tokens tokens = Tokenizer::tokenize(lines[i]);
            if (tokens.empty() || tokens[0].type == TokenType::Comment) {
                continue;
            }

            try {
                if (tokens.size() > 1 && tokens[1].type == TokenType::Assignment) {
                    VariableAssignment::assign(tokens);
                }
                else if (tokens[0].type == TokenType::Print) {
                    PrintStatement::print(tokens);
                }
                else if (tokens[0].type == TokenType::If) {
                    i = execute(lines, i, tokens);
                }
                else if (tokens[0].type == TokenType::Import) {
                    ModuleImporter::import(tokens);
                }
                else if (functions.count(tokens[0].value)) {
                    FunctionExecution::execute(tokens);
                }
                else {
                    double value = ExpressionEvaluator::evaluate(tokens);
                    std::cout << "Resultado de la expresión: " << value << std::endl;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
    }

} // namespace Clot