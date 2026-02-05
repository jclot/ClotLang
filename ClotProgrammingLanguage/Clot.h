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

#ifndef CLOT_H
#define CLOT_H

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <stack>
#include <cmath>
#include <variant>

using VariableName = std::string;
using FunctionName = std::string;
using Line = std::string;
using List = std::vector<std::variant<long, double, std::string, bool>>;
using Object = std::vector<std::pair<std::string, std::variant<double, std::string, bool, List>>>;

namespace Clot {

	extern std::map<VariableName, double> DOUBLE;
	extern std::map<VariableName, std::string> STRING;
	extern std::map<VariableName, bool> BOOL;
	extern std::map<VariableName, List> LIST;
	extern std::map<VariableName, Object> OBJECT;
	extern std::map<VariableName, int> INT;
	extern std::map<VariableName, long long> LONG;
	extern std::map<VariableName, unsigned char> BYTE;

	struct Function {
		std::vector<VariableName> parameters;
		std::vector<Line> body;
		std::vector<bool> isReference;
	};

	extern std::map<FunctionName, Function> functions;

	enum class TokenType {
		Identifier,
		Number,
		String,
		Boolean,
		List,
		Object,
		Long,
		Byte,
		Assignment,
		Plus,
		Minus,
		Multiply,
		Divide,
		Modulo,
		Power,
		And,
		Or,
		Not,
		Equal,
		NotEqual,
		Less,
		LessEqual,
		Greater,
		GreaterEqual,
		PlusEqual,
		MinusEqual,
		Comma,
		LeftParen,
		RightParen,
		LeftBracket,
		RightBracket,
		LeftBrace,
		RightBrace,
		Colon,
		SemiColon,
		Func,
		EndFunc,
		Return,
		Print,
		Import,
		If,
		Else,
		EndIf,
		Comment,
		Ampersand,
		Unknown
	};

	struct Token {
		TokenType type;
		std::string value;
	};

	using Tokens = std::vector<Token>;

	void Interpret(const std::string& filename);

} // namespace Clot

#endif // CLOT_H