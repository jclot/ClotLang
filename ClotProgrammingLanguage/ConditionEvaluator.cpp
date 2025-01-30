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

#include "ConditionEvaluator.h"
#include "ExpressionEvaluator.h"

namespace Clot {

	bool ConditionEvaluator::evaluate(const Tokens& tokens) {
		if (tokens.size() < 3) {
			throw std::runtime_error("Error en if: formato invalido.");
		}

		double left = ExpressionEvaluator::evaluate({ tokens.begin() + 1, tokens.end() - 1 });
		double right = ExpressionEvaluator::evaluate({ tokens.end() - 1, tokens.end() });

		switch (tokens[0].type) {
			case TokenType::Equal: return left == right;
			case TokenType::NotEqual: return left != right; 
			case TokenType::Less: return left < right;
			case TokenType::LessEqual: return left <= right;
			case TokenType::Greater: return left > right;
			case TokenType::GreaterEqual: return left >= right;
			default: throw std::runtime_error("Error en if: operador invalido.");
		}
	}
} // namespace Clot
