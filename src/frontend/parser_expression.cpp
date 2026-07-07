#include "clot/frontend/parser.hpp"

#include <charconv>
#include <cctype>
#include <memory>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

#include "clot/frontend/tokenizer.hpp"

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

            if (Match(TokenKind::KeywordIn)) {
                auto rhs = ParseTerm();
                if (rhs == nullptr) {
                    return nullptr;
                }
                expression = std::make_unique<BinaryExpr>(BinaryOp::In, std::move(expression), std::move(rhs));
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
                std::vector<CallArgument> arguments;
                const Token& open_paren = Previous();
                if (!ParseCallArguments(open_paren.column, &arguments)) {
                    return nullptr;
                }

                if (auto* variable = dynamic_cast<VariableExpr*>(expression.get())) {
                    expression = std::make_unique<CallExpr>(variable->name, std::move(arguments));
                    continue;
                }

                auto* member_access = dynamic_cast<IndexExpr*>(expression.get());
                auto* member_name_expr =
                    member_access == nullptr ? nullptr : dynamic_cast<StringExpr*>(member_access->index.get());
                if (member_access == nullptr || member_name_expr == nullptr) {
                    Fail(open_paren.column, "Solo se puede invocar funciones usando un identificador.");
                    return nullptr;
                }

                const std::string member_name = member_name_expr->value;
                std::unique_ptr<Expr> receiver = std::move(member_access->collection);

                if (member_name == "append") {
                    std::vector<CallArgument> append_arguments;
                    append_arguments.reserve(arguments.size() + 1);
                    CallArgument receiver_argument;
                    receiver_argument.value = std::move(receiver);
                    append_arguments.push_back(std::move(receiver_argument));
                    for (auto& argument : arguments) {
                        append_arguments.push_back(std::move(argument));
                    }

                    expression = std::make_unique<CallExpr>("__list_append__", std::move(append_arguments));
                    continue;
                }

                auto* receiver_variable = dynamic_cast<VariableExpr*>(receiver.get());
                if (receiver_variable != nullptr) {
                    expression = std::make_unique<CallExpr>(
                        receiver_variable->name + "." + member_name,
                        std::move(arguments));
                    continue;
                }

                std::vector<CallArgument> member_arguments;
                member_arguments.reserve(arguments.size() + 2);

                CallArgument receiver_argument;
                receiver_argument.value = std::move(receiver);
                member_arguments.push_back(std::move(receiver_argument));

                CallArgument member_name_argument;
                member_name_argument.value = std::make_unique<StringExpr>(member_name);
                member_arguments.push_back(std::move(member_name_argument));

                for (auto& argument : arguments) {
                    member_arguments.push_back(std::move(argument));
                }

                expression = std::make_unique<CallExpr>(
                    "__member_call__",
                    std::move(member_arguments));
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

            if (Match(TokenKind::Dot)) {
                if (IsAtEnd()) {
                    const Token& token = Previous();
                    Fail(token.column, "Falta identificador despues de '.'.");
                    return nullptr;
                }

                const Token member = Advance();
                if (member.kind != TokenKind::Identifier) {
                    Fail(member.column, "Se esperaba identificador despues de '.'.");
                    return nullptr;
                }

                expression = std::make_unique<IndexExpr>(
                    std::move(expression),
                    std::make_unique<StringExpr>(member.lexeme));
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
            const bool is_integer_literal = token.lexeme.find('.') == std::string::npos;
            if (is_integer_literal) {
                std::optional<long long> exact_integer64;
                long long parsed = 0;
                const char* begin = token.lexeme.data();
                const char* end = begin + token.lexeme.size();
                const auto parsed_result = std::from_chars(begin, end, parsed);
                if (parsed_result.ec == std::errc() && parsed_result.ptr == end) {
                    exact_integer64 = parsed;
                } else if (parsed_result.ec != std::errc::result_out_of_range) {
                    Fail(token.column, "Numero invalido: '" + token.lexeme + "'.");
                    return nullptr;
                }
                return std::make_unique<NumberExpr>(0.0, token.lexeme, true, exact_integer64);
            }

            try {
                return std::make_unique<NumberExpr>(std::stod(token.lexeme), token.lexeme, false, std::nullopt);
            } catch (...) {
                Fail(token.column, "Numero invalido: '" + token.lexeme + "'.");
                return nullptr;
            }
        }

        if (token.kind == TokenKind::String) {
            // Plain strings are fully literal: braces and '$' carry no special
            // meaning. Interpolation lives exclusively in f-strings.
            return std::make_unique<StringExpr>(token.lexeme);
        }

        if (token.kind == TokenKind::FString) {
            return ParseFString(token);
        }

        if (token.kind == TokenKind::Char) {
            if (token.lexeme.size() != 1) {
                Fail(token.column, "Literal char invalido.");
                return nullptr;
            }
            return std::make_unique<CharExpr>(token.lexeme[0]);
        }

        if (token.kind == TokenKind::Boolean) {
            return std::make_unique<BoolExpr>(token.lexeme == "true");
        }

        if (token.kind == TokenKind::KeywordNull) {
            return std::make_unique<NullExpr>();
        }

        if (token.kind == TokenKind::Identifier) {
            return std::make_unique<VariableExpr>(token.lexeme);
        }

        if (token.kind == TokenKind::KeywordTuple ||
            token.kind == TokenKind::KeywordSet ||
            token.kind == TokenKind::KeywordMap) {
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

    bool ParseCallArguments(std::size_t open_paren_column, std::vector<CallArgument>* out_arguments) {
        if (out_arguments == nullptr) {
            return false;
        }
        out_arguments->clear();

        if (!Check(TokenKind::RightParen)) {
            while (true) {
                CallArgument argument;
                if (Match(TokenKind::Ampersand)) {
                    argument.by_reference = true;
                }

                argument.value = ParseLogicalOr();
                if (argument.value == nullptr) {
                    return false;
                }

                out_arguments->push_back(std::move(argument));
                if (!Match(TokenKind::Comma)) {
                    break;
                }
            }
        }

        if (!Match(TokenKind::RightParen)) {
            const Token& token = IsAtEnd() ? Previous() : Peek();
            const std::size_t column = token.column == 0 ? open_paren_column : token.column;
            Fail(column, "Falta ')' al cerrar llamada de funcion.");
            return false;
        }

        return true;
    }

    static void AppendInterpolatedPiece(std::unique_ptr<Expr>* accumulator, std::unique_ptr<Expr> piece) {
        if (accumulator == nullptr || piece == nullptr) {
            return;
        }

        if (*accumulator == nullptr) {
            *accumulator = std::move(piece);
            return;
        }

        *accumulator = std::make_unique<BinaryExpr>(
            BinaryOp::Add,
            std::move(*accumulator),
            std::move(piece));
    }

    // Parses an f-string literal, desugaring `{expr}` placeholders into string
    // concatenation. Literal braces are written `{{` and `}}`.
    std::unique_ptr<Expr> ParseFString(const Token& token) {
        const std::string& raw_text = token.lexeme;
        if (raw_text.find('{') == std::string::npos && raw_text.find('}') == std::string::npos) {
            return std::make_unique<StringExpr>(raw_text);
        }

        std::unique_ptr<Expr> expression;
        std::string pending_text;
        std::size_t cursor = 0;

        auto flush_pending_text = [&]() {
            if (pending_text.empty()) {
                return;
            }
            AppendInterpolatedPiece(&expression, std::make_unique<StringExpr>(pending_text));
            pending_text.clear();
        };

        while (cursor < raw_text.size()) {
            const char current = raw_text[cursor];

            if (current == '{' && cursor + 1 < raw_text.size() && raw_text[cursor + 1] == '{') {
                pending_text.push_back('{');
                cursor += 2;
                continue;
            }

            if (current == '}' && cursor + 1 < raw_text.size() && raw_text[cursor + 1] == '}') {
                pending_text.push_back('}');
                cursor += 2;
                continue;
            }

            if (current == '{') {
                flush_pending_text();

                std::size_t expression_cursor = cursor + 1;
                int brace_depth = 1;
                bool in_string = false;
                bool escaped = false;
                char string_delimiter = '\0';

                for (; expression_cursor < raw_text.size(); ++expression_cursor) {
                    const char candidate = raw_text[expression_cursor];
                    if (in_string) {
                        if (escaped) {
                            escaped = false;
                            continue;
                        }
                        if (candidate == '\\') {
                            escaped = true;
                            continue;
                        }
                        if (candidate == string_delimiter) {
                            in_string = false;
                        }
                        continue;
                    }

                    if (candidate == '"' || candidate == '\'') {
                        in_string = true;
                        string_delimiter = candidate;
                        continue;
                    }

                    if (candidate == '{') {
                        ++brace_depth;
                        continue;
                    }

                    if (candidate == '}') {
                        --brace_depth;
                        if (brace_depth == 0) {
                            break;
                        }
                    }
                }

                if (expression_cursor >= raw_text.size() || brace_depth != 0) {
                    Fail(token.column, "Interpolacion de string incompleta: falta '}'.");
                    return nullptr;
                }

                std::string interpolation_text =
                    raw_text.substr(cursor + 1, expression_cursor - (cursor + 1));
                std::size_t first = 0;
                while (first < interpolation_text.size() &&
                       std::isspace(static_cast<unsigned char>(interpolation_text[first]))) {
                    ++first;
                }

                std::size_t last = interpolation_text.size();
                while (last > first &&
                       std::isspace(static_cast<unsigned char>(interpolation_text[last - 1]))) {
                    --last;
                }

                if (first == last) {
                    Fail(token.column, "Interpolacion de string vacia.");
                    return nullptr;
                }

                interpolation_text = interpolation_text.substr(first, last - first);
                std::vector<Token> interpolation_tokens = Tokenizer::TokenizeLine(interpolation_text);
                if (interpolation_tokens.empty()) {
                    Fail(token.column, "Interpolacion de string vacia.");
                    return nullptr;
                }

                if (interpolation_tokens[0].kind == TokenKind::Unknown) {
                    Fail(token.column, "Interpolacion de string invalida: '" + interpolation_tokens[0].lexeme + "'.");
                    return nullptr;
                }

                ExpressionParser interpolation_parser(line_number_, std::move(interpolation_tokens));
                std::unique_ptr<Expr> interpolation_expression;
                if (!interpolation_parser.Parse(&interpolation_expression, out_error_)) {
                    return nullptr;
                }

                if (expression == nullptr) {
                    expression = std::make_unique<StringExpr>("");
                }
                AppendInterpolatedPiece(&expression, std::move(interpolation_expression));
                cursor = expression_cursor + 1;
                continue;
            }

            if (current == '}') {
                Fail(token.column, "Interpolacion de string invalida: '}' sin apertura.");
                return nullptr;
            }

            pending_text.push_back(current);
            ++cursor;
        }

        flush_pending_text();
        if (expression == nullptr) {
            return std::make_unique<StringExpr>(raw_text);
        }

        return expression;
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
