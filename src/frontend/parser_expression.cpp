#include "clot/frontend/parser.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace clot::frontend {

namespace {

class ExpressionParser {
public:
    ExpressionParser(std::size_t line_number, std::vector<Token> tokens)
        : line_number_(line_number), tokens_(std::move(tokens)) {}

    bool Parse(std::unique_ptr<Expr>* out_expression, Diagnostic* out_error) {
        out_error_ = out_error;

        auto expression = ParseLogicalOr();
        if (expression == nullptr) {
            return false;
        }

        if (!IsAtEnd()) {
            const Token& token = Peek();
            return Fail(token.column, "Token inesperado en expresion: '" + token.lexeme + "'.");
        }

        *out_expression = std::move(expression);
        return true;
    }

private:
    std::unique_ptr<Expr> ParseLogicalOr() {
        auto expression = ParseLogicalAnd();
        while (expression != nullptr && Match(TokenKind::Or)) {
            auto rhs = ParseLogicalAnd();
            if (rhs == nullptr) {
                return nullptr;
            }
            expression = std::make_unique<BinaryExpr>(BinaryOp::LogicalOr, std::move(expression), std::move(rhs));
        }
        return expression;
    }

    std::unique_ptr<Expr> ParseLogicalAnd() {
        auto expression = ParseEquality();
        while (expression != nullptr && Match(TokenKind::And)) {
            auto rhs = ParseEquality();
            if (rhs == nullptr) {
                return nullptr;
            }
            expression = std::make_unique<BinaryExpr>(BinaryOp::LogicalAnd, std::move(expression), std::move(rhs));
        }
        return expression;
    }

    std::unique_ptr<Expr> ParseEquality() {
        auto expression = ParseComparison();
        while (expression != nullptr) {
            if (Match(TokenKind::EqualEqual)) {
                auto rhs = ParseComparison();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::Equal, std::move(expression), std::move(rhs));
                continue;
            }

            if (Match(TokenKind::BangEqual)) {
                auto rhs = ParseComparison();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::NotEqual, std::move(expression), std::move(rhs));
                continue;
            }

            break;
        }
        return expression;
    }

    std::unique_ptr<Expr> ParseComparison() {
        auto expression = ParseTerm();
        while (expression != nullptr) {
            if (Match(TokenKind::Less)) {
                auto rhs = ParseTerm();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::Less, std::move(expression), std::move(rhs));
                continue;
            }

            if (Match(TokenKind::LessEqual)) {
                auto rhs = ParseTerm();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::LessEqual, std::move(expression), std::move(rhs));
                continue;
            }

            if (Match(TokenKind::Greater)) {
                auto rhs = ParseTerm();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::Greater, std::move(expression), std::move(rhs));
                continue;
            }

            if (Match(TokenKind::GreaterEqual)) {
                auto rhs = ParseTerm();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::GreaterEqual, std::move(expression), std::move(rhs));
                continue;
            }

            break;
        }
        return expression;
    }

    std::unique_ptr<Expr> ParseTerm() {
        auto expression = ParseFactor();
        while (expression != nullptr) {
            if (Match(TokenKind::Plus)) {
                auto rhs = ParseFactor();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::Add, std::move(expression), std::move(rhs));
                continue;
            }

            if (Match(TokenKind::Minus)) {
                auto rhs = ParseFactor();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::Subtract, std::move(expression), std::move(rhs));
                continue;
            }

            break;
        }
        return expression;
    }

    std::unique_ptr<Expr> ParseFactor() {
        auto expression = ParsePower();
        while (expression != nullptr) {
            if (Match(TokenKind::Star)) {
                auto rhs = ParsePower();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::Multiply, std::move(expression), std::move(rhs));
                continue;
            }

            if (Match(TokenKind::Slash)) {
                auto rhs = ParsePower();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::Divide, std::move(expression), std::move(rhs));
                continue;
            }

            if (Match(TokenKind::Percent)) {
                auto rhs = ParsePower();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::Modulo, std::move(expression), std::move(rhs));
                continue;
            }

            break;
        }

        return expression;
    }

    std::unique_ptr<Expr> ParsePower() {
        auto expression = ParseUnary();
        if (expression == nullptr) {
            return nullptr;
        }

        if (Match(TokenKind::Caret)) {
            auto rhs = ParsePower();
            if (rhs == nullptr) {
                return nullptr;
            }
            expression = std::make_unique<BinaryExpr>(BinaryOp::Power, std::move(expression), std::move(rhs));
        }

        return expression;
    }

    std::unique_ptr<Expr> ParseUnary() {
        if (Match(TokenKind::Not)) {
            auto operand = ParseUnary();
            if (operand == nullptr) {
                return nullptr;
            }
            return std::make_unique<UnaryExpr>(UnaryOp::LogicalNot, std::move(operand));
        }

        if (Match(TokenKind::Minus)) {
            auto operand = ParseUnary();
            if (operand == nullptr) {
                return nullptr;
            }
            return std::make_unique<UnaryExpr>(UnaryOp::Negate, std::move(operand));
        }

        if (Match(TokenKind::Plus)) {
            auto operand = ParseUnary();
            if (operand == nullptr) {
                return nullptr;
            }
            return std::make_unique<UnaryExpr>(UnaryOp::Plus, std::move(operand));
        }

        return ParsePostfix();
    }

    std::unique_ptr<Expr> ParsePostfix() {
        auto expression = ParsePrimary();
        if (expression == nullptr) {
            return nullptr;
        }

        while (!IsAtEnd()) {
            if (Match(TokenKind::LeftParen)) {
                auto* variable = dynamic_cast<VariableExpr*>(expression.get());
                if (variable == nullptr) {
                    const Token& token = Previous();
                    Fail(token.column, "Solo se puede invocar funciones usando un identificador.");
                    return nullptr;
                }

                std::vector<CallArgument> arguments;
                if (!Check(TokenKind::RightParen)) {
                    while (true) {
                        CallArgument argument;
                        if (Match(TokenKind::Ampersand)) {
                            argument.by_reference = true;
                        }

                        argument.value = ParseLogicalOr();
                        if (argument.value == nullptr) {
                            return nullptr;
                        }

                        arguments.push_back(std::move(argument));
                        if (!Match(TokenKind::Comma)) {
                            break;
                        }
                    }
                }

                if (!Match(TokenKind::RightParen)) {
                    const Token& token = IsAtEnd() ? Previous() : Peek();
                    Fail(token.column, "Falta ')' al cerrar llamada de funcion.");
                    return nullptr;
                }

                const std::string callee_name = variable->name;
                expression = std::make_unique<CallExpr>(callee_name, std::move(arguments));
                continue;
            }

            if (Match(TokenKind::LeftBracket)) {
                auto index = ParseLogicalOr();
                if (index == nullptr) {
                    return nullptr;
                }

                if (!Match(TokenKind::RightBracket)) {
                    const Token& token = IsAtEnd() ? Previous() : Peek();
                    Fail(token.column, "Falta ']' al cerrar indice de lista.");
                    return nullptr;
                }

                expression = std::make_unique<IndexExpr>(std::move(expression), std::move(index));
                continue;
            }

            break;
        }

        return expression;
    }

    std::unique_ptr<Expr> ParsePrimary() {
        if (IsAtEnd()) {
            Fail(1, "Expresion incompleta.");
            return nullptr;
        }

        const Token token = Advance();

        if (token.kind == TokenKind::Number) {
            try {
                return std::make_unique<NumberExpr>(std::stod(token.lexeme));
            } catch (...) {
                Fail(token.column, "Numero invalido: '" + token.lexeme + "'.");
                return nullptr;
            }
        }

        if (token.kind == TokenKind::String) {
            return std::make_unique<StringExpr>(token.lexeme);
        }

        if (token.kind == TokenKind::Boolean) {
            return std::make_unique<BoolExpr>(token.lexeme == "true");
        }

        if (token.kind == TokenKind::Identifier) {
            return std::make_unique<VariableExpr>(token.lexeme);
        }

        if (token.kind == TokenKind::LeftParen) {
            auto expression = ParseLogicalOr();
            if (expression == nullptr) {
                return nullptr;
            }

            if (!Match(TokenKind::RightParen)) {
                const Token& next = IsAtEnd() ? token : Peek();
                Fail(next.column, "Falta ')' en expresion.");
                return nullptr;
            }

            return expression;
        }

        if (token.kind == TokenKind::LeftBracket) {
            std::vector<std::unique_ptr<Expr>> elements;

            if (!Check(TokenKind::RightBracket)) {
                while (true) {
                    auto element = ParseLogicalOr();
                    if (element == nullptr) {
                        return nullptr;
                    }

                    elements.push_back(std::move(element));
                    if (!Match(TokenKind::Comma)) {
                        break;
                    }
                }
            }

            if (!Match(TokenKind::RightBracket)) {
                const Token& next = IsAtEnd() ? token : Peek();
                Fail(next.column, "Falta ']' al cerrar literal de lista.");
                return nullptr;
            }

            return std::make_unique<ListExpr>(std::move(elements));
        }

        if (token.kind == TokenKind::LeftBrace) {
            std::vector<ObjectEntryExpr> entries;

            if (!Check(TokenKind::RightBrace)) {
                while (true) {
                    if (IsAtEnd()) {
                        Fail(token.column, "Literal de objeto incompleto.");
                        return nullptr;
                    }

                    const Token key_token = Advance();
                    if (key_token.kind != TokenKind::Identifier && key_token.kind != TokenKind::String) {
                        Fail(key_token.column, "Clave invalida en objeto: '" + key_token.lexeme + "'.");
                        return nullptr;
                    }

                    if (!Match(TokenKind::Colon)) {
                        const Token& next = IsAtEnd() ? key_token : Peek();
                        Fail(next.column, "Falta ':' despues de clave de objeto.");
                        return nullptr;
                    }

                    auto value_expr = ParseLogicalOr();
                    if (value_expr == nullptr) {
                        return nullptr;
                    }

                    entries.push_back(ObjectEntryExpr{key_token.lexeme, std::move(value_expr)});
                    if (!Match(TokenKind::Comma)) {
                        break;
                    }
                }
            }

            if (!Match(TokenKind::RightBrace)) {
                const Token& next = IsAtEnd() ? token : Peek();
                Fail(next.column, "Falta '}' al cerrar literal de objeto.");
                return nullptr;
            }

            return std::make_unique<ObjectExpr>(std::move(entries));
        }

        Fail(token.column, "Token no soportado en expresion: '" + token.lexeme + "'.");
        return nullptr;
    }

    bool IsAtEnd() const {
        return cursor_ >= tokens_.size();
    }

    const Token& Peek() const {
        return tokens_[cursor_];
    }

    const Token& Previous() const {
        return tokens_[cursor_ - 1];
    }

    Token Advance() {
        const Token token = tokens_[cursor_];
        ++cursor_;
        return token;
    }

    bool Check(TokenKind expected) const {
        return !IsAtEnd() && tokens_[cursor_].kind == expected;
    }

    bool Match(TokenKind expected) {
        if (!Check(expected)) {
            return false;
        }
        ++cursor_;
        return true;
    }

    bool Fail(std::size_t column, const std::string& message) {
        if (out_error_ != nullptr) {
            out_error_->line = line_number_;
            out_error_->column = column;
            out_error_->message = message;
        }
        return false;
    }

    std::size_t line_number_ = 0;
    std::vector<Token> tokens_;
    std::size_t cursor_ = 0;
    Diagnostic* out_error_ = nullptr;
};

}  // namespace

bool Parser::ParseExpression(
    std::size_t line_number,
    std::vector<Token> tokens,
    std::unique_ptr<Expr>* out_expr,
    Diagnostic* out_error) const {
    ExpressionParser parser(line_number, std::move(tokens));
    return parser.Parse(out_expr, out_error);
}

}  // namespace clot::frontend
