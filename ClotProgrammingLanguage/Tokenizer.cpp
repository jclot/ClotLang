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

#include "Tokenizer.h"

namespace Clot {

	Tokens Tokenizer::tokenize(const Line& line) {

		Tokens tokens;

		std::string trimmed = line;
		trimmed.erase(0, trimmed.find_first_not_of(" \t"));
		if (trimmed.substr(0, 2) == "//") {
			tokens.push_back({ TokenType::Comment, trimmed });
			return tokens;
		}

		std::string currentToken;
		bool insideQuotes = false;

		for (char currentChar : line) {

			if (currentChar == '"') {
				insideQuotes = !insideQuotes;
				currentToken += currentChar;
			}
			else if (insideQuotes) {
				currentToken += currentChar;
			}
			else if (isSpecialChar(currentChar)) {
				if (!currentToken.empty()) {
					tokens.push_back(createToken(currentToken));
					currentToken.clear();
				}
				tokens.push_back(createToken(std::string(1, currentChar)));
			}
			else if (std::isspace(currentChar)) {
				if (!currentToken.empty()) {
					tokens.push_back(createToken(currentToken));
					currentToken.clear();
				}
			}
			else {
				currentToken += currentChar;
			}
		}

		/*for (size_t i = 0; i < line.size(); ++i) {
			char currentChar = line[i];
			if (currentChar == '"') {
				insideQuotes = !insideQuotes;
				currentToken += currentChar;
			}
			else if (insideQuotes) {
				currentToken += currentChar;
			}
			else if (isSpecialChar(currentChar)) {
				if (!currentToken.empty()) {
					tokens.push_back(createToken(currentToken));
					currentToken.clear();
				}
				if (i + 1 < line.size() && isSpecialChar(line[i + 1])) {
					currentToken += currentChar;
					currentToken += line[++i];
					tokens.push_back(createToken(currentToken));
					currentToken.clear();
				}
				else {
					tokens.push_back(createToken(std::string(1, currentChar)));
				}
			}
			else if (std::isspace(currentChar)) {
				if (!currentToken.empty()) {
					tokens.push_back(createToken(currentToken));
					currentToken.clear();
				}
			}
			else {
				currentToken += currentChar;
			}
		}*/

		if (!currentToken.empty()) {
			tokens.push_back(createToken(currentToken));
		}

		return tokens;
	}

	bool Tokenizer::isSpecialChar(char c) {
		return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ',' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^' || c == '=' || c == '!' || c == '<' || c == '>' || c == ':' || c == '&' || c == '|' || c == ';';
	}

	Token Tokenizer::createToken(const std::string& str) {
		if (str == "long") return { TokenType::Long, str };
		if (str == "byte") return { TokenType::Byte, str };
		if (str == "=") return { TokenType::Assignment, str };
		if (str == "+") return { TokenType::Plus, str };
		if (str == "-") return { TokenType::Minus, str };
		if (str == "*") return { TokenType::Multiply, str };
		if (str == "/") return { TokenType::Divide, str };
		if (str == "%") return { TokenType::Modulo, str };
		if (str == "^") return { TokenType::Power, str };
		if (str == "&&") return { TokenType::And, str };
		if (str == "||") return { TokenType::Or, str };
		if (str == "!") return { TokenType::Not, str };
		if (str == "==") return { TokenType::Equal, str };
		if (str == "!=") return { TokenType::NotEqual, str };
		if (str == "<") return { TokenType::Less, str };
		if (str == "<=") return { TokenType::LessEqual, str };
		if (str == ">") return { TokenType::Greater, str };
		if (str == ">=") return { TokenType::GreaterEqual, str };
		if (str == "+=") return { TokenType::PlusEqual, str };
		if (str == "-=") return { TokenType::MinusEqual, str };
		if (str == ",") return { TokenType::Comma, str };
		if (str == "(") return { TokenType::LeftParen, str };
		if (str == ")") return { TokenType::RightParen, str };
		if (str == "[") return { TokenType::LeftBracket, str };
		if (str == "]") return { TokenType::RightBracket, str };
		if (str == "{") return { TokenType::LeftBrace, str };
		if (str == "}") return { TokenType::RightBrace, str };
		if (str == ":") return { TokenType::Colon, str };
		if (str == ";") return { TokenType::SemiColon, str };
		if (str == "func") return { TokenType::Func, str };
		if (str == "endfunc") return { TokenType::EndFunc, str };
		if (str == "return") return { TokenType::Return, str };
		if (str == "print") return { TokenType::Print, str };
		if (str == "import") return { TokenType::Import, str };
		if (str == "if") return { TokenType::If, str };
		if (str == "else") return { TokenType::Else, str };
		if (str == "endif") return { TokenType::EndIf, str };
		if (str == "//") return { TokenType::Comment, str };
		if (str == "&") return { TokenType::Ampersand, str };
		if (str == "true" || str == "false") return { TokenType::Boolean, str };
		if (isNumber(str)) return { TokenType::Number, str };
		if (isString(str)) return { TokenType::String, str };
		return { TokenType::Identifier, str };
	}

	bool Tokenizer::isNumber(const std::string& s) {
		return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c) && c != '.'; }) == s.end();
	}

	bool Tokenizer::isString(const std::string& s) {
		return s.size() >= 2 && s.front() == '"' && s.back() == '"';
	}

} // namespace Clot
