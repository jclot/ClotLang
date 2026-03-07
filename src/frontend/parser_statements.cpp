#include "clot/frontend/parser.hpp"

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clot/frontend/tokenizer.hpp"
#include "parser_support.hpp"

namespace clot::frontend {

namespace {

std::string ToLowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool TryParseTypeHintToken(const Token& token, TypeHint* out_type_hint) {
    if (out_type_hint == nullptr) {
        return false;
    }

    switch (token.kind) {
    case TokenKind::KeywordInt:
        *out_type_hint = TypeHint::Int;
        return true;
    case TokenKind::KeywordDouble:
        *out_type_hint = TypeHint::Double;
        return true;
    case TokenKind::KeywordFloat:
        *out_type_hint = TypeHint::Float;
        return true;
    case TokenKind::KeywordDecimal:
        *out_type_hint = TypeHint::Decimal;
        return true;
    case TokenKind::KeywordLong:
        *out_type_hint = TypeHint::Long;
        return true;
    case TokenKind::KeywordByte:
        *out_type_hint = TypeHint::Byte;
        return true;
    case TokenKind::KeywordChar:
        *out_type_hint = TypeHint::Char;
        return true;
    case TokenKind::KeywordTuple:
        *out_type_hint = TypeHint::Tuple;
        return true;
    case TokenKind::KeywordSet:
        *out_type_hint = TypeHint::Set;
        return true;
    case TokenKind::KeywordMap:
        *out_type_hint = TypeHint::Map;
        return true;
    case TokenKind::KeywordFunctionType:
        *out_type_hint = TypeHint::Function;
        return true;
    case TokenKind::KeywordNull:
        *out_type_hint = TypeHint::Null;
        return true;
    case TokenKind::Identifier:
        break;
    default:
        return false;
    }

    const std::string lowered = ToLowerAscii(token.lexeme);
    if (lowered == "int") {
        *out_type_hint = TypeHint::Int;
        return true;
    }
    if (lowered == "double") {
        *out_type_hint = TypeHint::Double;
        return true;
    }
    if (lowered == "float") {
        *out_type_hint = TypeHint::Float;
        return true;
    }
    if (lowered == "decimal") {
        *out_type_hint = TypeHint::Decimal;
        return true;
    }
    if (lowered == "long") {
        *out_type_hint = TypeHint::Long;
        return true;
    }
    if (lowered == "byte") {
        *out_type_hint = TypeHint::Byte;
        return true;
    }
    if (lowered == "char") {
        *out_type_hint = TypeHint::Char;
        return true;
    }
    if (lowered == "tuple") {
        *out_type_hint = TypeHint::Tuple;
        return true;
    }
    if (lowered == "set") {
        *out_type_hint = TypeHint::Set;
        return true;
    }
    if (lowered == "map") {
        *out_type_hint = TypeHint::Map;
        return true;
    }
    if (lowered == "function") {
        *out_type_hint = TypeHint::Function;
        return true;
    }
    if (lowered == "string") {
        *out_type_hint = TypeHint::String;
        return true;
    }
    if (lowered == "bool") {
        *out_type_hint = TypeHint::Bool;
        return true;
    }
    if (lowered == "list") {
        *out_type_hint = TypeHint::List;
        return true;
    }
    if (lowered == "object") {
        *out_type_hint = TypeHint::Object;
        return true;
    }
    if (lowered == "null") {
        *out_type_hint = TypeHint::Null;
        return true;
    }
    if (lowered == "any" || lowered == "dynamic") {
        *out_type_hint = TypeHint::Inferred;
        return true;
    }

    return false;
}

}  // namespace

bool Parser::ParseAssignment(std::size_t* line_index, const std::vector<Token>& tokens,
                             std::vector<std::unique_ptr<Statement>>* out_statements, Diagnostic* out_error) const {
    DeclarationType declaration_type = DeclarationType::Inferred;
    std::size_t cursor = 0;

    if (tokens[cursor].kind == TokenKind::KeywordInt) {
        declaration_type = DeclarationType::Int;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordDouble) {
        declaration_type = DeclarationType::Double;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordFloat) {
        declaration_type = DeclarationType::Float;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordDecimal) {
        declaration_type = DeclarationType::Decimal;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordLong) {
        declaration_type = DeclarationType::Long;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordByte) {
        declaration_type = DeclarationType::Byte;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordChar) {
        declaration_type = DeclarationType::Char;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordTuple) {
        declaration_type = DeclarationType::Tuple;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordSet) {
        declaration_type = DeclarationType::Set;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordMap) {
        declaration_type = DeclarationType::Map;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordFunctionType) {
        declaration_type = DeclarationType::Function;
        ++cursor;
    }

    if (cursor >= tokens.size() || tokens[cursor].kind != TokenKind::Identifier) {
        const std::size_t bad_cursor = cursor < tokens.size() ? cursor : 0;
        *out_error = MakeError(*line_index + 1, tokens[bad_cursor].column, "Se esperaba un identificador.");
        return false;
    }

    const std::string variable_name = tokens[cursor].lexeme;
    ++cursor;

    if (cursor >= tokens.size()) {
        *out_error = MakeError(*line_index + 1, tokens.back().column, "Falta operador de asignacion.");
        return false;
    }

    AssignmentOp assignment_op = AssignmentOp::Set;
    if (tokens[cursor].kind == TokenKind::Assign) {
        assignment_op = AssignmentOp::Set;
    } else if (tokens[cursor].kind == TokenKind::PlusEqual) {
        assignment_op = AssignmentOp::AddAssign;
    } else if (tokens[cursor].kind == TokenKind::MinusEqual) {
        assignment_op = AssignmentOp::SubAssign;
    } else {
        *out_error = MakeError(*line_index + 1, tokens[cursor].column, "Operador de asignacion no valido.");
        return false;
    }

    if (declaration_type != DeclarationType::Inferred && assignment_op != AssignmentOp::Set) {
        *out_error = MakeError(*line_index + 1, tokens[cursor].column, "Las declaraciones tipadas solo aceptan '='.");
        return false;
    }

    ++cursor;

    if (tokens.back().kind != TokenKind::Semicolon) {
        *out_error = MakeError(*line_index + 1, tokens.back().column, "Falta ';' al final de la asignacion.");
        return false;
    }

    if (cursor >= tokens.size() - 1) {
        *out_error = MakeError(*line_index + 1, tokens.back().column, "Falta expresion en la asignacion.");
        return false;
    }

    std::vector<Token> expression_tokens(tokens.begin() + static_cast<std::ptrdiff_t>(cursor), tokens.end() - 1);

    std::unique_ptr<Expr> expression;
    if (!ParseExpression(*line_index + 1, std::move(expression_tokens), &expression, out_error)) {
        return false;
    }

    out_statements->push_back(
        std::make_unique<AssignmentStmt>(variable_name, assignment_op, declaration_type, std::move(expression)));

    ++(*line_index);
    return true;
}

bool Parser::ParsePrint(std::size_t* line_index, const std::vector<Token>& tokens,
                        std::vector<std::unique_ptr<Statement>>* out_statements, Diagnostic* out_error) const {
    const bool append_newline = tokens[0].kind == TokenKind::KeywordPrintln;

    if (tokens.size() < 4) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Instruccion de salida incompleta.");
        return false;
    }

    if (tokens[1].kind != TokenKind::LeftParen) {
        *out_error = MakeError(*line_index + 1, tokens[1].column, "Se esperaba '(' despues de la instruccion de salida.");
        return false;
    }

    if (tokens.back().kind != TokenKind::Semicolon) {
        *out_error = MakeError(*line_index + 1, tokens.back().column, "Falta ';' al final de la instruccion de salida.");
        return false;
    }

    int depth = 0;
    std::size_t closing_paren_index = tokens.size();
    for (std::size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i].kind == TokenKind::LeftParen) {
            ++depth;
        } else if (tokens[i].kind == TokenKind::RightParen) {
            --depth;
            if (depth == 0) {
                closing_paren_index = i;
                break;
            }
        }
    }

    if (closing_paren_index == tokens.size() || closing_paren_index != tokens.size() - 2) {
        *out_error = MakeError(*line_index + 1, tokens.back().column,
                               "La instruccion de salida requiere cerrar ')' antes de ';'.");
        return false;
    }

    if (!append_newline && closing_paren_index <= 2) {
        *out_error = MakeError(*line_index + 1, tokens[1].column, "print requiere una expresion interna.");
        return false;
    }

    std::unique_ptr<Expr> expression;
    if (closing_paren_index > 2) {
        std::vector<Token> expression_tokens(tokens.begin() + 2,
                                             tokens.begin() + static_cast<std::ptrdiff_t>(closing_paren_index));
        if (!ParseExpression(*line_index + 1, std::move(expression_tokens), &expression, out_error)) {
            return false;
        }
    }

    out_statements->push_back(std::make_unique<PrintStmt>(std::move(expression), append_newline));
    ++(*line_index);
    return true;
}

bool Parser::ParseIf(std::size_t* line_index, const std::vector<Token>& tokens,
                     std::vector<std::unique_ptr<Statement>>* out_statements, Diagnostic* out_error) const {
    if (tokens.size() < 3) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Instruccion if incompleta.");
        return false;
    }

    if (tokens.back().kind != TokenKind::Colon) {
        *out_error = MakeError(*line_index + 1, tokens.back().column, "Falta ':' al final del if.");
        return false;
    }

    std::vector<Token> condition_tokens(tokens.begin() + 1, tokens.end() - 1);
    std::unique_ptr<Expr> condition;
    if (!ParseExpression(*line_index + 1, std::move(condition_tokens), &condition, out_error)) {
        return false;
    }

    auto if_statement = std::make_unique<IfStmt>(std::move(condition));

    ++(*line_index);
    if (!ParseBlock(line_index, true, &if_statement->then_branch, out_error)) {
        return false;
    }

    if (*line_index >= lines_.size()) {
        *out_error = MakeError(*line_index, 1, "Falta 'endif' para cerrar el bloque if.");
        return false;
    }

    std::vector<Token> control_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
    if (control_tokens.empty()) {
        *out_error = MakeError(*line_index + 1, 1, "Se esperaba 'else:' o 'endif'.");
        return false;
    }

    if (control_tokens[0].kind == TokenKind::KeywordElse) {
        if (control_tokens.back().kind != TokenKind::Colon) {
            *out_error = MakeError(*line_index + 1, control_tokens.back().column, "Falta ':' al final de else.");
            return false;
        }

        ++(*line_index);
        if (!ParseBlock(line_index, true, &if_statement->else_branch, out_error)) {
            return false;
        }

        if (*line_index >= lines_.size()) {
            *out_error = MakeError(*line_index, 1, "Falta 'endif' para cerrar el bloque else.");
            return false;
        }

        control_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
        if (control_tokens.empty() || control_tokens[0].kind != TokenKind::KeywordEndIf) {
            *out_error = MakeError(*line_index + 1, 1, "Se esperaba 'endif' despues de else.");
            return false;
        }
    }

    if (control_tokens[0].kind != TokenKind::KeywordEndIf) {
        *out_error = MakeError(*line_index + 1, control_tokens[0].column, "Se esperaba 'endif'.");
        return false;
    }

    out_statements->push_back(std::move(if_statement));
    ++(*line_index);
    return true;
}

bool Parser::ParseFunctionDeclaration(std::size_t* line_index, const std::vector<Token>& tokens,
                                      std::vector<std::unique_ptr<Statement>>* out_statements,
                                      Diagnostic* out_error) const {
    if (tokens.size() < 5) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Declaracion de funcion incompleta.");
        return false;
    }

    TypeHint return_type_hint = TypeHint::Inferred;
    std::size_t name_index = 1;
    if (tokens.size() >= 6) {
        TypeHint parsed_return_type = TypeHint::Inferred;
        if (TryParseTypeHintToken(tokens[1], &parsed_return_type) &&
            tokens[2].kind == TokenKind::Identifier &&
            tokens[3].kind == TokenKind::LeftParen) {
            return_type_hint = parsed_return_type;
            name_index = 2;
        }
    }

    if (name_index >= tokens.size() || tokens[name_index].kind != TokenKind::Identifier) {
        const std::size_t error_index = name_index < tokens.size() ? name_index : 1;
        *out_error = MakeError(*line_index + 1, tokens[error_index].column, "Falta nombre de funcion valido.");
        return false;
    }

    if (name_index + 1 >= tokens.size() || tokens[name_index + 1].kind != TokenKind::LeftParen) {
        const std::size_t error_index = name_index + 1 < tokens.size() ? name_index + 1 : tokens.size() - 1;
        *out_error = MakeError(
            *line_index + 1,
            tokens[error_index].column,
            "Se esperaba '(' en la declaracion de funcion.");
        return false;
    }

    if (tokens.back().kind != TokenKind::Colon) {
        *out_error =
            MakeError(*line_index + 1, tokens.back().column, "Falta ':' al final de la declaracion de funcion.");
        return false;
    }

    const std::string function_name = tokens[name_index].lexeme;
    std::vector<FunctionParam> params;
    bool seen_default_parameter = false;

    std::size_t cursor = name_index + 2;
    while (cursor < tokens.size()) {
        if (tokens[cursor].kind == TokenKind::RightParen) {
            ++cursor;
            break;
        }

        bool by_reference = false;
        if (tokens[cursor].kind == TokenKind::Ampersand) {
            by_reference = true;
            ++cursor;
        }

        if (cursor >= tokens.size() || tokens[cursor].kind != TokenKind::Identifier) {
            const std::size_t error_cursor = cursor < tokens.size() ? cursor : tokens.size() - 1;
            *out_error = MakeError(*line_index + 1, tokens[error_cursor].column,
                                   "Parametro invalido en declaracion de funcion.");
            return false;
        }

        const std::string param_name = tokens[cursor].lexeme;
        ++cursor;

        TypeHint param_type_hint = TypeHint::Inferred;
        if (cursor < tokens.size() && tokens[cursor].kind == TokenKind::Colon) {
            ++cursor;
            if (cursor >= tokens.size()) {
                *out_error = MakeError(
                    *line_index + 1,
                    tokens.back().column,
                    "Falta tipo de parametro despues de ':'.");
                return false;
            }

            if (!TryParseTypeHintToken(tokens[cursor], &param_type_hint)) {
                *out_error = MakeError(
                    *line_index + 1,
                    tokens[cursor].column,
                    "Tipo de parametro no reconocido: '" + tokens[cursor].lexeme + "'.");
                return false;
            }
            ++cursor;
        }

        std::unique_ptr<Expr> default_expr;
        if (cursor < tokens.size() && tokens[cursor].kind == TokenKind::Assign) {
            ++cursor;
            if (cursor >= tokens.size()) {
                *out_error = MakeError(
                    *line_index + 1,
                    tokens.back().column,
                    "Falta expresion de valor por defecto despues de '='.");
                return false;
            }

            const std::size_t default_start = cursor;
            int paren_depth = 0;
            int bracket_depth = 0;
            int brace_depth = 0;
            for (; cursor < tokens.size(); ++cursor) {
                const TokenKind kind = tokens[cursor].kind;

                if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0 &&
                    (kind == TokenKind::Comma || kind == TokenKind::RightParen)) {
                    break;
                }

                if (kind == TokenKind::LeftParen) {
                    ++paren_depth;
                } else if (kind == TokenKind::RightParen) {
                    --paren_depth;
                } else if (kind == TokenKind::LeftBracket) {
                    ++bracket_depth;
                } else if (kind == TokenKind::RightBracket) {
                    --bracket_depth;
                } else if (kind == TokenKind::LeftBrace) {
                    ++brace_depth;
                } else if (kind == TokenKind::RightBrace) {
                    --brace_depth;
                }
            }

            if (cursor == default_start) {
                *out_error = MakeError(
                    *line_index + 1,
                    tokens[default_start - 1].column,
                    "Falta expresion de valor por defecto despues de '='.");
                return false;
            }

            std::vector<Token> default_tokens(
                tokens.begin() + static_cast<std::ptrdiff_t>(default_start),
                tokens.begin() + static_cast<std::ptrdiff_t>(cursor));
            if (!ParseExpression(*line_index + 1, std::move(default_tokens), &default_expr, out_error)) {
                return false;
            }
            seen_default_parameter = true;
        } else if (seen_default_parameter) {
            *out_error = MakeError(
                *line_index + 1,
                tokens[cursor].column,
                "Los parametros sin valor por defecto no pueden ir despues de parametros con default.");
            return false;
        }

        if (by_reference && default_expr != nullptr) {
            *out_error = MakeError(
                *line_index + 1,
                tokens[cursor - 1].column,
                "Los parametros por referencia no aceptan valor por defecto.");
            return false;
        }

        params.push_back(FunctionParam{
            param_name,
            by_reference,
            param_type_hint,
            std::move(default_expr),
        });

        if (cursor < tokens.size() && tokens[cursor].kind == TokenKind::Comma) {
            ++cursor;
            continue;
        }

        if (cursor < tokens.size() && tokens[cursor].kind == TokenKind::RightParen) {
            continue;
        }

        if (cursor >= tokens.size()) {
            break;
        }

        *out_error =
            MakeError(*line_index + 1, tokens[cursor].column, "Se esperaba ',' o ')' en parametros de funcion.");
        return false;
    }

    if (cursor >= tokens.size() || tokens[cursor].kind != TokenKind::Colon) {
        const std::size_t error_cursor = cursor < tokens.size() ? cursor : tokens.size() - 1;
        *out_error = MakeError(*line_index + 1, tokens[error_cursor].column,
                               "Declaracion de funcion invalida: falta ':' final.");
        return false;
    }

    if (cursor != tokens.size() - 1) {
        *out_error =
            MakeError(*line_index + 1, tokens[cursor + 1].column, "Tokens extra despues de declaracion de funcion.");
        return false;
    }

    std::vector<std::unique_ptr<Statement>> body;
    ++(*line_index);

    while (*line_index < lines_.size()) {
        const std::vector<Token> body_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
        if (body_tokens.empty()) {
            ++(*line_index);
            continue;
        }

        if (body_tokens[0].kind == TokenKind::KeywordEndFunc) {
            if (body_tokens.size() != 1) {
                *out_error =
                    MakeError(*line_index + 1, body_tokens[1].column, "'endfunc' no acepta tokens adicionales.");
                return false;
            }

            out_statements->push_back(
                std::make_unique<FunctionDeclStmt>(
                    function_name,
                    return_type_hint,
                    std::move(params),
                    std::move(body)));
            ++(*line_index);
            return true;
        }

        if (!ParseStatement(line_index, body_tokens, &body, out_error)) {
            return false;
        }
    }

    *out_error = MakeError(*line_index, 1, "Falta 'endfunc' para cerrar la funcion '" + function_name + "'.");
    return false;
}

bool Parser::ParseInterfaceDeclaration(std::size_t* line_index, const std::vector<Token>& tokens,
                                       std::vector<std::unique_ptr<Statement>>* out_statements,
                                       Diagnostic* out_error) const {
    if (tokens.size() < 3 || tokens[1].kind != TokenKind::Identifier || tokens.back().kind != TokenKind::Colon) {
        *out_error = MakeError(
            *line_index + 1,
            tokens[0].column,
            "Formato invalido en interface. Use: interface Nombre:");
        return false;
    }

    if (tokens.size() != 3) {
        *out_error = MakeError(
            *line_index + 1,
            tokens[2].column,
            "Tokens extra en declaracion de interface.");
        return false;
    }

    const std::string interface_name = tokens[1].lexeme;
    std::vector<InterfaceMethodSignature> methods;

    ++(*line_index);
    while (*line_index < lines_.size()) {
        const std::vector<Token> body_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
        if (body_tokens.empty()) {
            ++(*line_index);
            continue;
        }

        if (body_tokens[0].kind == TokenKind::KeywordEndInterface) {
            if (body_tokens.size() != 1) {
                *out_error =
                    MakeError(*line_index + 1, body_tokens[1].column, "'endinterface' no acepta tokens adicionales.");
                return false;
            }

            out_statements->push_back(std::make_unique<InterfaceDeclStmt>(interface_name, std::move(methods)));
            ++(*line_index);
            return true;
        }

        if (body_tokens[0].kind != TokenKind::KeywordFunc) {
            *out_error = MakeError(
                *line_index + 1,
                body_tokens[0].column,
                "Solo se permiten firmas 'func ...;' dentro de interface.");
            return false;
        }

        if (body_tokens.back().kind != TokenKind::Semicolon) {
            *out_error = MakeError(
                *line_index + 1,
                body_tokens.back().column,
                "Las firmas en interface deben terminar en ';'.");
            return false;
        }

        TypeHint return_type_hint = TypeHint::Inferred;
        std::size_t name_index = 1;
        if (body_tokens.size() >= 6) {
            TypeHint parsed_return_type = TypeHint::Inferred;
            if (TryParseTypeHintToken(body_tokens[1], &parsed_return_type) &&
                body_tokens[2].kind == TokenKind::Identifier &&
                body_tokens[3].kind == TokenKind::LeftParen) {
                return_type_hint = parsed_return_type;
                name_index = 2;
            }
        }

        if (name_index >= body_tokens.size() || body_tokens[name_index].kind != TokenKind::Identifier) {
            const std::size_t error_index = name_index < body_tokens.size() ? name_index : 1;
            *out_error = MakeError(
                *line_index + 1,
                body_tokens[error_index].column,
                "Firma invalida en interface: falta nombre de metodo.");
            return false;
        }

        if (name_index + 1 >= body_tokens.size() || body_tokens[name_index + 1].kind != TokenKind::LeftParen) {
            *out_error = MakeError(
                *line_index + 1,
                body_tokens[name_index].column,
                "Firma invalida en interface: se esperaba '('.");
            return false;
        }

        std::vector<FunctionParam> params;
        std::size_t cursor = name_index + 2;
        while (cursor < body_tokens.size()) {
            if (body_tokens[cursor].kind == TokenKind::RightParen) {
                ++cursor;
                break;
            }

            bool by_reference = false;
            if (body_tokens[cursor].kind == TokenKind::Ampersand) {
                by_reference = true;
                ++cursor;
            }

            if (cursor >= body_tokens.size() || body_tokens[cursor].kind != TokenKind::Identifier) {
                const std::size_t bad = cursor < body_tokens.size() ? cursor : body_tokens.size() - 1;
                *out_error = MakeError(
                    *line_index + 1,
                    body_tokens[bad].column,
                    "Parametro invalido en firma de interface.");
                return false;
            }

            const std::string param_name = body_tokens[cursor].lexeme;
            ++cursor;

            TypeHint param_type_hint = TypeHint::Inferred;
            if (cursor < body_tokens.size() && body_tokens[cursor].kind == TokenKind::Colon) {
                ++cursor;
                if (cursor >= body_tokens.size()) {
                    *out_error = MakeError(
                        *line_index + 1,
                        body_tokens.back().column,
                        "Falta tipo de parametro despues de ':'.");
                    return false;
                }
                if (!TryParseTypeHintToken(body_tokens[cursor], &param_type_hint)) {
                    *out_error = MakeError(
                        *line_index + 1,
                        body_tokens[cursor].column,
                        "Tipo de parametro no reconocido en interface.");
                    return false;
                }
                ++cursor;
            }

            if (cursor < body_tokens.size() && body_tokens[cursor].kind == TokenKind::Assign) {
                *out_error = MakeError(
                    *line_index + 1,
                    body_tokens[cursor].column,
                    "Las firmas de interface no aceptan valores por defecto.");
                return false;
            }

            params.push_back(FunctionParam{
                param_name,
                by_reference,
                param_type_hint,
                nullptr,
            });

            if (cursor < body_tokens.size() && body_tokens[cursor].kind == TokenKind::Comma) {
                ++cursor;
                continue;
            }

            if (cursor < body_tokens.size() && body_tokens[cursor].kind == TokenKind::RightParen) {
                continue;
            }

            *out_error = MakeError(
                *line_index + 1,
                body_tokens[cursor].column,
                "Se esperaba ',' o ')' en firma de interface.");
            return false;
        }

        if (cursor >= body_tokens.size() || body_tokens[cursor].kind != TokenKind::Semicolon) {
            const std::size_t error_cursor = cursor < body_tokens.size() ? cursor : body_tokens.size() - 1;
            *out_error = MakeError(
                *line_index + 1,
                body_tokens[error_cursor].column,
                "Falta ';' al final de firma de interface.");
            return false;
        }

        methods.push_back(InterfaceMethodSignature{
            body_tokens[name_index].lexeme,
            return_type_hint,
            std::move(params),
        });

        ++(*line_index);
    }

    *out_error = MakeError(*line_index, 1, "Falta 'endinterface' para cerrar la interface '" + interface_name + "'.");
    return false;
}

bool Parser::ParseClassDeclaration(std::size_t* line_index, const std::vector<Token>& tokens,
                                   std::vector<std::unique_ptr<Statement>>* out_statements,
                                   Diagnostic* out_error) const {
    if (tokens.size() < 3 || tokens[1].kind != TokenKind::Identifier || tokens.back().kind != TokenKind::Colon) {
        *out_error = MakeError(
            *line_index + 1,
            tokens[0].column,
            "Formato invalido en class. Use: class Nombre:");
        return false;
    }

    std::string class_name = tokens[1].lexeme;
    std::string base_class;
    std::vector<std::string> interfaces;

    std::size_t cursor = 2;
    while (cursor + 1 < tokens.size()) {
        if (tokens[cursor].kind == TokenKind::KeywordExtends) {
            ++cursor;
            if (cursor >= tokens.size() - 1 || tokens[cursor].kind != TokenKind::Identifier) {
                *out_error = MakeError(
                    *line_index + 1,
                    tokens[cursor < tokens.size() ? cursor : tokens.size() - 1].column,
                    "Se esperaba clase base valida despues de 'extends'.");
                return false;
            }
            base_class = tokens[cursor].lexeme;
            ++cursor;
            continue;
        }

        if (tokens[cursor].kind == TokenKind::KeywordImplements) {
            ++cursor;
            if (cursor >= tokens.size() - 1 || tokens[cursor].kind != TokenKind::Identifier) {
                *out_error = MakeError(
                    *line_index + 1,
                    tokens[cursor < tokens.size() ? cursor : tokens.size() - 1].column,
                    "Se esperaba interface valida despues de 'implements'.");
                return false;
            }

            while (cursor < tokens.size() - 1) {
                if (tokens[cursor].kind != TokenKind::Identifier) {
                    *out_error = MakeError(
                        *line_index + 1,
                        tokens[cursor].column,
                        "Nombre de interface invalido en 'implements'.");
                    return false;
                }
                interfaces.push_back(tokens[cursor].lexeme);
                ++cursor;

                if (cursor < tokens.size() - 1 && tokens[cursor].kind == TokenKind::Comma) {
                    ++cursor;
                    continue;
                }
                break;
            }
            continue;
        }

        *out_error = MakeError(
            *line_index + 1,
            tokens[cursor].column,
            "Token no esperado en cabecera de class: '" + tokens[cursor].lexeme + "'.");
        return false;
    }

    auto parse_params = [&](const std::vector<Token>& header_tokens,
                            std::size_t params_start,
                            std::vector<FunctionParam>* out_params) -> bool {
        std::size_t local_cursor = params_start;
        bool seen_default_parameter = false;
        out_params->clear();

        while (local_cursor < header_tokens.size()) {
            if (header_tokens[local_cursor].kind == TokenKind::RightParen) {
                ++local_cursor;
                break;
            }

            bool by_reference = false;
            if (header_tokens[local_cursor].kind == TokenKind::Ampersand) {
                by_reference = true;
                ++local_cursor;
            }

            if (local_cursor >= header_tokens.size() || header_tokens[local_cursor].kind != TokenKind::Identifier) {
                const std::size_t bad = local_cursor < header_tokens.size() ? local_cursor : header_tokens.size() - 1;
                *out_error = MakeError(
                    *line_index + 1,
                    header_tokens[bad].column,
                    "Parametro invalido en declaracion.");
                return false;
            }

            const std::string param_name = header_tokens[local_cursor].lexeme;
            ++local_cursor;

            TypeHint param_type_hint = TypeHint::Inferred;
            if (local_cursor < header_tokens.size() && header_tokens[local_cursor].kind == TokenKind::Colon) {
                ++local_cursor;
                if (local_cursor >= header_tokens.size()) {
                    *out_error = MakeError(
                        *line_index + 1,
                        header_tokens.back().column,
                        "Falta tipo de parametro despues de ':'.");
                    return false;
                }

                if (!TryParseTypeHintToken(header_tokens[local_cursor], &param_type_hint)) {
                    *out_error = MakeError(
                        *line_index + 1,
                        header_tokens[local_cursor].column,
                        "Tipo de parametro no reconocido: '" + header_tokens[local_cursor].lexeme + "'.");
                    return false;
                }
                ++local_cursor;
            }

            std::unique_ptr<Expr> default_expr;
            if (local_cursor < header_tokens.size() && header_tokens[local_cursor].kind == TokenKind::Assign) {
                ++local_cursor;
                if (local_cursor >= header_tokens.size()) {
                    *out_error = MakeError(
                        *line_index + 1,
                        header_tokens.back().column,
                        "Falta expresion de valor por defecto despues de '='.");
                    return false;
                }

                const std::size_t default_start = local_cursor;
                int paren_depth = 0;
                int bracket_depth = 0;
                int brace_depth = 0;
                for (; local_cursor < header_tokens.size(); ++local_cursor) {
                    const TokenKind kind = header_tokens[local_cursor].kind;
                    if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0 &&
                        (kind == TokenKind::Comma || kind == TokenKind::RightParen)) {
                        break;
                    }

                    if (kind == TokenKind::LeftParen) {
                        ++paren_depth;
                    } else if (kind == TokenKind::RightParen) {
                        --paren_depth;
                    } else if (kind == TokenKind::LeftBracket) {
                        ++bracket_depth;
                    } else if (kind == TokenKind::RightBracket) {
                        --bracket_depth;
                    } else if (kind == TokenKind::LeftBrace) {
                        ++brace_depth;
                    } else if (kind == TokenKind::RightBrace) {
                        --brace_depth;
                    }
                }

                if (local_cursor == default_start) {
                    *out_error = MakeError(
                        *line_index + 1,
                        header_tokens[default_start - 1].column,
                        "Falta expresion de valor por defecto despues de '='.");
                    return false;
                }

                std::vector<Token> default_tokens(
                    header_tokens.begin() + static_cast<std::ptrdiff_t>(default_start),
                    header_tokens.begin() + static_cast<std::ptrdiff_t>(local_cursor));
                if (!ParseExpression(*line_index + 1, std::move(default_tokens), &default_expr, out_error)) {
                    return false;
                }
                seen_default_parameter = true;
            } else if (seen_default_parameter) {
                *out_error = MakeError(
                    *line_index + 1,
                    header_tokens[local_cursor].column,
                    "Los parametros sin valor por defecto no pueden ir despues de parametros con default.");
                return false;
            }

            if (by_reference && default_expr != nullptr) {
                *out_error = MakeError(
                    *line_index + 1,
                    header_tokens[local_cursor - 1].column,
                    "Los parametros por referencia no aceptan valor por defecto.");
                return false;
            }

            out_params->push_back(FunctionParam{
                param_name,
                by_reference,
                param_type_hint,
                std::move(default_expr),
            });

            if (local_cursor < header_tokens.size() && header_tokens[local_cursor].kind == TokenKind::Comma) {
                ++local_cursor;
                continue;
            }

            if (local_cursor < header_tokens.size() && header_tokens[local_cursor].kind == TokenKind::RightParen) {
                continue;
            }

            if (local_cursor >= header_tokens.size()) {
                break;
            }

            *out_error = MakeError(
                *line_index + 1,
                header_tokens[local_cursor].column,
                "Se esperaba ',' o ')' en parametros.");
            return false;
        }

        if (local_cursor >= header_tokens.size() || header_tokens[local_cursor].kind != TokenKind::Colon) {
            const std::size_t bad = local_cursor < header_tokens.size() ? local_cursor : header_tokens.size() - 1;
            *out_error = MakeError(*line_index + 1, header_tokens[bad].column, "Falta ':' al final de declaracion.");
            return false;
        }

        if (local_cursor != header_tokens.size() - 1) {
            *out_error = MakeError(
                *line_index + 1,
                header_tokens[local_cursor + 1].column,
                "Tokens extra despues de declaracion.");
            return false;
        }

        return true;
    };

    auto parse_body_until = [&](TokenKind end_token,
                                std::vector<std::unique_ptr<Statement>>* out_body,
                                const std::string& end_lexeme) -> bool {
        out_body->clear();
        ++(*line_index);
        while (*line_index < lines_.size()) {
            const std::vector<Token> body_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
            if (body_tokens.empty()) {
                ++(*line_index);
                continue;
            }

            if (body_tokens[0].kind == end_token) {
                if (body_tokens.size() != 1) {
                    *out_error = MakeError(
                        *line_index + 1,
                        body_tokens[1].column,
                        "'" + end_lexeme + "' no acepta tokens adicionales.");
                    return false;
                }
                ++(*line_index);
                return true;
            }

            if (!ParseStatement(line_index, body_tokens, out_body, out_error)) {
                return false;
            }
        }

        *out_error = MakeError(*line_index, 1, "Falta '" + end_lexeme + "' para cerrar bloque.");
        return false;
    };

    std::vector<ClassFieldDecl> fields;
    bool has_constructor = false;
    bool constructor_is_private = false;
    std::vector<FunctionParam> constructor_params;
    std::vector<std::unique_ptr<Statement>> constructor_body;
    std::vector<ClassMethodDecl> methods;
    std::vector<ClassAccessorDecl> accessors;

    ++(*line_index);
    while (*line_index < lines_.size()) {
        const std::vector<Token> member_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
        if (member_tokens.empty()) {
            ++(*line_index);
            continue;
        }

        if (member_tokens[0].kind == TokenKind::KeywordEndClass) {
            if (member_tokens.size() != 1) {
                *out_error =
                    MakeError(*line_index + 1, member_tokens[1].column, "'endclass' no acepta tokens adicionales.");
                return false;
            }

            out_statements->push_back(std::make_unique<ClassDeclStmt>(
                class_name,
                std::move(base_class),
                std::move(interfaces),
                std::move(fields),
                constructor_is_private,
                std::move(constructor_params),
                std::move(constructor_body),
                std::move(methods),
                std::move(accessors)));
            ++(*line_index);
            return true;
        }

        std::size_t member_cursor = 0;
        MemberVisibility visibility = MemberVisibility::Public;
        bool is_static = false;
        bool is_readonly = false;
        bool is_override = false;

        while (member_cursor < member_tokens.size()) {
            const TokenKind kind = member_tokens[member_cursor].kind;
            if (kind == TokenKind::KeywordPublic) {
                visibility = MemberVisibility::Public;
                ++member_cursor;
                continue;
            }
            if (kind == TokenKind::KeywordPrivate) {
                visibility = MemberVisibility::Private;
                ++member_cursor;
                continue;
            }
            if (kind == TokenKind::KeywordStatic) {
                is_static = true;
                ++member_cursor;
                continue;
            }
            if (kind == TokenKind::KeywordReadonly) {
                is_readonly = true;
                ++member_cursor;
                continue;
            }
            if (kind == TokenKind::KeywordOverride) {
                is_override = true;
                ++member_cursor;
                continue;
            }
            break;
        }

        if (member_cursor >= member_tokens.size()) {
            *out_error = MakeError(*line_index + 1, member_tokens.back().column, "Declaracion de miembro vacia.");
            return false;
        }

        const TokenKind member_kind = member_tokens[member_cursor].kind;

        if (member_kind == TokenKind::KeywordConstructor) {
            if (is_static || is_readonly || is_override) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "constructor no acepta modificadores static/readonly/override.");
                return false;
            }
            if (has_constructor) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "Solo se permite un constructor por class.");
                return false;
            }
            if (member_cursor + 1 >= member_tokens.size() || member_tokens[member_cursor + 1].kind != TokenKind::LeftParen) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "Se esperaba '(' en constructor.");
                return false;
            }

            std::vector<FunctionParam> parsed_params;
            if (!parse_params(member_tokens, member_cursor + 2, &parsed_params)) {
                return false;
            }

            std::vector<std::unique_ptr<Statement>> parsed_body;
            if (!parse_body_until(TokenKind::KeywordEndConstructor, &parsed_body, "endconstructor")) {
                return false;
            }

            has_constructor = true;
            constructor_is_private = visibility == MemberVisibility::Private;
            constructor_params = std::move(parsed_params);
            constructor_body = std::move(parsed_body);
            continue;
        }

        if (member_kind == TokenKind::KeywordGet) {
            if (is_static || is_readonly || is_override) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "get no acepta modificadores static/readonly/override.");
                return false;
            }

            if (member_cursor + 4 >= member_tokens.size() ||
                member_tokens[member_cursor + 1].kind != TokenKind::Identifier ||
                member_tokens[member_cursor + 2].kind != TokenKind::LeftParen ||
                member_tokens[member_cursor + 3].kind != TokenKind::RightParen ||
                member_tokens.back().kind != TokenKind::Colon) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "Formato invalido en get. Use: get nombre():");
                return false;
            }

            std::vector<std::unique_ptr<Statement>> parsed_body;
            if (!parse_body_until(TokenKind::KeywordEndGet, &parsed_body, "endget")) {
                return false;
            }

            ClassAccessorDecl accessor;
            accessor.name = member_tokens[member_cursor + 1].lexeme;
            accessor.is_setter = false;
            accessor.visibility = visibility;
            accessor.body = std::move(parsed_body);
            accessors.push_back(std::move(accessor));
            continue;
        }

        if (member_kind == TokenKind::KeywordSet &&
            member_cursor + 2 < member_tokens.size() &&
            member_tokens[member_cursor + 1].kind == TokenKind::Identifier &&
            member_tokens[member_cursor + 2].kind == TokenKind::LeftParen) {
            if (is_static || is_readonly || is_override) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "set no acepta modificadores static/readonly/override.");
                return false;
            }

            std::size_t setter_cursor = member_cursor + 3;
            if (setter_cursor >= member_tokens.size() || member_tokens[setter_cursor].kind != TokenKind::Identifier) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "set requiere un parametro.");
                return false;
            }
            const std::string setter_param_name = member_tokens[setter_cursor].lexeme;
            ++setter_cursor;

            TypeHint setter_param_type = TypeHint::Inferred;
            if (setter_cursor < member_tokens.size() && member_tokens[setter_cursor].kind == TokenKind::Colon) {
                ++setter_cursor;
                if (setter_cursor >= member_tokens.size()) {
                    *out_error = MakeError(
                        *line_index + 1,
                        member_tokens.back().column,
                        "Falta tipo en parametro de set.");
                    return false;
                }
                if (!TryParseTypeHintToken(member_tokens[setter_cursor], &setter_param_type)) {
                    *out_error = MakeError(
                        *line_index + 1,
                        member_tokens[setter_cursor].column,
                        "Tipo invalido en parametro de set.");
                    return false;
                }
                ++setter_cursor;
            }

            if (setter_cursor >= member_tokens.size() || member_tokens[setter_cursor].kind != TokenKind::RightParen) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "set requiere cerrar ')' en su parametro.");
                return false;
            }
            ++setter_cursor;

            if (setter_cursor >= member_tokens.size() || member_tokens[setter_cursor].kind != TokenKind::Colon) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "Falta ':' al final de set.");
                return false;
            }

            if (setter_cursor != member_tokens.size() - 1) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[setter_cursor + 1].column,
                    "Tokens extra despues de set.");
                return false;
            }

            std::vector<std::unique_ptr<Statement>> parsed_body;
            if (!parse_body_until(TokenKind::KeywordEndSet, &parsed_body, "endset")) {
                return false;
            }

            ClassAccessorDecl accessor;
            accessor.name = member_tokens[member_cursor + 1].lexeme;
            accessor.is_setter = true;
            accessor.setter_param_name = setter_param_name;
            accessor.setter_param_type = setter_param_type;
            accessor.visibility = visibility;
            accessor.body = std::move(parsed_body);
            accessors.push_back(std::move(accessor));
            continue;
        }

        if (member_kind == TokenKind::KeywordFunc) {
            if (is_readonly) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[member_cursor].column,
                    "readonly no aplica a metodos.");
                return false;
            }

            std::vector<Token> func_tokens(
                member_tokens.begin() + static_cast<std::ptrdiff_t>(member_cursor),
                member_tokens.end());
            std::vector<std::unique_ptr<Statement>> parsed_function_statements;
            if (!ParseFunctionDeclaration(line_index, func_tokens, &parsed_function_statements, out_error)) {
                return false;
            }

            if (parsed_function_statements.size() != 1) {
                *out_error = MakeError(*line_index + 1, member_tokens[member_cursor].column,
                                       "Error interno parseando metodo de class.");
                return false;
            }

            auto* function_ptr = dynamic_cast<FunctionDeclStmt*>(parsed_function_statements[0].release());
            if (function_ptr == nullptr) {
                *out_error = MakeError(*line_index + 1, member_tokens[member_cursor].column,
                                       "Error interno parseando metodo de class.");
                return false;
            }

            std::unique_ptr<FunctionDeclStmt> function_decl(function_ptr);
            ClassMethodDecl method;
            method.name = function_decl->name;
            method.return_type = function_decl->return_type;
            method.params = std::move(function_decl->params);
            method.body = std::move(function_decl->body);
            method.visibility = visibility;
            method.is_static = is_static;
            method.is_override = is_override;
            methods.push_back(std::move(method));
            continue;
        }

        if (is_override) {
            *out_error = MakeError(
                *line_index + 1,
                member_tokens[member_cursor].column,
                "override solo aplica a metodos.");
            return false;
        }

        // Campo.
        TypeHint field_type = TypeHint::Inferred;
        std::size_t field_cursor = member_cursor;
        TypeHint parsed_field_type = TypeHint::Inferred;
        if (field_cursor + 1 < member_tokens.size() &&
            TryParseTypeHintToken(member_tokens[field_cursor], &parsed_field_type) &&
            member_tokens[field_cursor + 1].kind == TokenKind::Identifier) {
            field_type = parsed_field_type;
            ++field_cursor;
        }

        if (field_cursor >= member_tokens.size() || member_tokens[field_cursor].kind != TokenKind::Identifier) {
            *out_error = MakeError(
                *line_index + 1,
                member_tokens[field_cursor < member_tokens.size() ? field_cursor : member_tokens.size() - 1].column,
                "Declaracion invalida de campo en class.");
            return false;
        }

        const std::string field_name = member_tokens[field_cursor].lexeme;
        ++field_cursor;

        std::unique_ptr<Expr> default_expr;
        if (field_cursor < member_tokens.size() && member_tokens[field_cursor].kind == TokenKind::Assign) {
            ++field_cursor;
            if (field_cursor >= member_tokens.size()) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens.back().column,
                    "Falta expresion de inicializacion en campo.");
                return false;
            }

            if (member_tokens.back().kind != TokenKind::Semicolon) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens.back().column,
                    "Falta ';' al final de campo.");
                return false;
            }

            std::vector<Token> expr_tokens(
                member_tokens.begin() + static_cast<std::ptrdiff_t>(field_cursor),
                member_tokens.end() - 1);
            if (!ParseExpression(*line_index + 1, std::move(expr_tokens), &default_expr, out_error)) {
                return false;
            }
        } else {
            if (member_tokens.back().kind != TokenKind::Semicolon) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens.back().column,
                    "Falta ';' al final de campo.");
                return false;
            }

            if (field_cursor != member_tokens.size() - 1) {
                *out_error = MakeError(
                    *line_index + 1,
                    member_tokens[field_cursor].column,
                    "Declaracion invalida de campo en class.");
                return false;
            }
        }

        fields.push_back(ClassFieldDecl{
            field_name,
            field_type,
            visibility,
            is_static,
            is_readonly,
            std::move(default_expr),
        });
        ++(*line_index);
    }

    *out_error = MakeError(*line_index, 1, "Falta 'endclass' para cerrar la class '" + class_name + "'.");
    return false;
}

bool Parser::ParseImport(std::size_t* line_index, const std::vector<Token>& tokens,
                         std::vector<std::unique_ptr<Statement>>* out_statements, Diagnostic* out_error) const {
    const auto invalid_import_format = [&]() {
        *out_error = MakeError(
            *line_index + 1,
            tokens[0].column,
            "Formato invalido en import. Use: import modulo;, import modulo as alias;, from modulo import simbolo; o from modulo import simbolo as alias;");
        return false;
    };

    if (tokens[0].kind == TokenKind::KeywordImport) {
        // import modulo;
        if (tokens.size() == 3 &&
            tokens[1].kind == TokenKind::Identifier &&
            tokens[2].kind == TokenKind::Semicolon) {
            out_statements->push_back(std::make_unique<ImportStmt>(
                ImportStmt::Style::Module,
                tokens[1].lexeme,
                std::string(),
                std::string(),
                std::string()));
            ++(*line_index);
            return true;
        }

        // import modulo as alias;
        if (tokens.size() == 5 &&
            tokens[1].kind == TokenKind::Identifier &&
            tokens[2].kind == TokenKind::Identifier &&
            tokens[2].lexeme == "as" &&
            tokens[3].kind == TokenKind::Identifier &&
            tokens[4].kind == TokenKind::Semicolon) {
            out_statements->push_back(std::make_unique<ImportStmt>(
                ImportStmt::Style::ModuleAlias,
                tokens[1].lexeme,
                tokens[3].lexeme,
                std::string(),
                std::string()));
            ++(*line_index);
            return true;
        }

        return invalid_import_format();
    }

    // from modulo import simbolo;
    if (tokens[0].kind == TokenKind::Identifier &&
        tokens[0].lexeme == "from") {
        if (tokens.size() == 5 &&
            tokens[1].kind == TokenKind::Identifier &&
            tokens[2].kind == TokenKind::KeywordImport &&
            tokens[3].kind == TokenKind::Identifier &&
            tokens[4].kind == TokenKind::Semicolon) {
            out_statements->push_back(std::make_unique<ImportStmt>(
                ImportStmt::Style::FromImport,
                tokens[1].lexeme,
                std::string(),
                tokens[3].lexeme,
                tokens[3].lexeme));
            ++(*line_index);
            return true;
        }

        // from modulo import simbolo as alias;
        if (tokens.size() == 7 &&
            tokens[1].kind == TokenKind::Identifier &&
            tokens[2].kind == TokenKind::KeywordImport &&
            tokens[3].kind == TokenKind::Identifier &&
            tokens[4].kind == TokenKind::Identifier &&
            tokens[4].lexeme == "as" &&
            tokens[5].kind == TokenKind::Identifier &&
            tokens[6].kind == TokenKind::Semicolon) {
            out_statements->push_back(std::make_unique<ImportStmt>(
                ImportStmt::Style::FromImport,
                tokens[1].lexeme,
                std::string(),
                tokens[3].lexeme,
                tokens[5].lexeme));
            ++(*line_index);
            return true;
        }

        return invalid_import_format();
    }

    return invalid_import_format();
}

bool Parser::ParseEnum(std::size_t* line_index, const std::vector<Token>& tokens,
                       std::vector<std::unique_ptr<Statement>>* out_statements, Diagnostic* out_error) const {
    if (tokens.size() < 2 || tokens[1].kind != TokenKind::Identifier) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Formato invalido en enum. Use: enum Nombre { A, B };");
        return false;
    }

    const std::string enum_name = tokens[1].lexeme;
    std::vector<std::string> members;

    std::size_t current_line = *line_index;
    std::vector<Token> current_tokens = tokens;
    std::size_t cursor = 2;
    bool opened = false;
    bool closed = false;
    bool expect_member = true;
    bool semicolon_consumed = false;

    while (true) {
        if (cursor >= current_tokens.size()) {
            ++current_line;
            if (current_line >= lines_.size()) {
                *out_error = MakeError(*line_index + 1, tokens[0].column,
                                       "Formato invalido en enum. Use: enum Nombre { A, B };");
                return false;
            }
            current_tokens = Tokenizer::TokenizeLine(lines_[current_line]);
            cursor = 0;
            if (current_tokens.empty()) {
                continue;
            }
            if (current_tokens[0].kind == TokenKind::Unknown) {
                *out_error = MakeError(current_line + 1, current_tokens[0].column,
                                       "Token no reconocido: '" + current_tokens[0].lexeme + "'.");
                return false;
            }
        }

        const Token& token = current_tokens[cursor];
        if (!opened) {
            if (token.kind != TokenKind::LeftBrace) {
                *out_error = MakeError(current_line + 1, token.column,
                                       "Formato invalido en enum. Use: enum Nombre { A, B };");
                return false;
            }
            opened = true;
            ++cursor;
            continue;
        }

        if (token.kind == TokenKind::RightBrace) {
            closed = true;
            ++cursor;

            if (cursor < current_tokens.size() && current_tokens[cursor].kind == TokenKind::Semicolon) {
                semicolon_consumed = true;
                ++cursor;
            }

            if (cursor < current_tokens.size()) {
                *out_error = MakeError(current_line + 1, current_tokens[cursor].column,
                                       "Tokens extra despues del cierre de enum.");
                return false;
            }
            break;
        }

        if (expect_member) {
            if (token.kind != TokenKind::Identifier) {
                *out_error = MakeError(current_line + 1, token.column, "Miembro invalido en enum.");
                return false;
            }
            members.push_back(token.lexeme);
            expect_member = false;
            ++cursor;
            continue;
        }

        if (token.kind == TokenKind::Comma) {
            expect_member = true;
            ++cursor;
            continue;
        }

        *out_error = MakeError(current_line + 1, token.column, "Se esperaba ',' o '}' en enum.");
        return false;
    }

    if (!closed) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Falta '}' para cerrar enum.");
        return false;
    }

    if (members.empty()) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Enum no puede estar vacio.");
        return false;
    }

    if (expect_member && !members.empty()) {
        *out_error = MakeError(current_line + 1, 1, "No se permite coma final en enum.");
        return false;
    }

    if (!semicolon_consumed) {
        std::size_t semicolon_line = current_line + 1;
        while (semicolon_line < lines_.size()) {
            const std::vector<Token> semicolon_tokens = Tokenizer::TokenizeLine(lines_[semicolon_line]);
            if (semicolon_tokens.empty()) {
                ++semicolon_line;
                continue;
            }

            if (semicolon_tokens[0].kind == TokenKind::Semicolon && semicolon_tokens.size() == 1) {
                semicolon_consumed = true;
                current_line = semicolon_line;
            }
            break;
        }
    }

    out_statements->push_back(std::make_unique<EnumDeclStmt>(enum_name, std::move(members)));
    *line_index = current_line + 1;
    return true;
}

bool Parser::ParseTry(std::size_t* line_index, const std::vector<Token>& tokens,
                      std::vector<std::unique_ptr<Statement>>* out_statements, Diagnostic* out_error) const {
    if (tokens.size() != 2 || tokens[1].kind != TokenKind::Colon) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Formato invalido en try. Use: try:");
        return false;
    }

    std::vector<std::unique_ptr<Statement>> try_branch;
    ++(*line_index);

    while (*line_index < lines_.size()) {
        const std::vector<Token> branch_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
        if (branch_tokens.empty()) {
            ++(*line_index);
            continue;
        }

        if (branch_tokens[0].kind == TokenKind::Unknown) {
            *out_error = MakeError(*line_index + 1, branch_tokens[0].column,
                                   "Token no reconocido: '" + branch_tokens[0].lexeme + "'.");
            return false;
        }

        if (branch_tokens[0].kind == TokenKind::KeywordCatch || branch_tokens[0].kind == TokenKind::KeywordEndTry) {
            break;
        }

        if (!ParseStatement(line_index, branch_tokens, &try_branch, out_error)) {
            return false;
        }
    }

    if (*line_index >= lines_.size()) {
        *out_error = MakeError(*line_index, 1, "Falta 'catch:' para cerrar bloque try.");
        return false;
    }

    std::vector<Token> control_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
    if (control_tokens.empty() || control_tokens[0].kind != TokenKind::KeywordCatch) {
        const std::size_t column = control_tokens.empty() ? 1 : control_tokens[0].column;
        *out_error = MakeError(*line_index + 1, column, "Se esperaba 'catch:' despues de try.");
        return false;
    }

    if (control_tokens.back().kind != TokenKind::Colon) {
        *out_error = MakeError(*line_index + 1, control_tokens.back().column, "Falta ':' al final de catch.");
        return false;
    }

    std::string catch_type;
    std::string error_binding;
    if (control_tokens.size() == 2) {
        // catch:
    } else if (control_tokens.size() == 5 && control_tokens[1].kind == TokenKind::LeftParen &&
               control_tokens[2].kind == TokenKind::Identifier && control_tokens[3].kind == TokenKind::RightParen) {
        const std::string candidate = control_tokens[2].lexeme;
        const bool looks_like_type =
            !candidate.empty() && std::isupper(static_cast<unsigned char>(candidate.front()));
        if (looks_like_type) {
            catch_type = candidate;
        } else {
            error_binding = candidate;
        }
    } else if (control_tokens.size() == 6 &&
               control_tokens[1].kind == TokenKind::LeftParen &&
               control_tokens[2].kind == TokenKind::Identifier &&
               control_tokens[3].kind == TokenKind::Identifier &&
               control_tokens[4].kind == TokenKind::RightParen) {
        catch_type = control_tokens[2].lexeme;
        error_binding = control_tokens[3].lexeme;
    } else {
        *out_error = MakeError(*line_index + 1, control_tokens[0].column,
                               "Formato invalido en catch. Use: catch:, catch(error):, catch(Tipo): o catch(Tipo error):");
        return false;
    }

    std::vector<std::unique_ptr<Statement>> catch_branch;
    ++(*line_index);

    while (*line_index < lines_.size()) {
        const std::vector<Token> branch_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
        if (branch_tokens.empty()) {
            ++(*line_index);
            continue;
        }

        if (branch_tokens[0].kind == TokenKind::Unknown) {
            *out_error = MakeError(*line_index + 1, branch_tokens[0].column,
                                   "Token no reconocido: '" + branch_tokens[0].lexeme + "'.");
            return false;
        }

        if (branch_tokens[0].kind == TokenKind::KeywordEndTry) {
            break;
        }

        if (branch_tokens[0].kind == TokenKind::KeywordCatch) {
            *out_error =
                MakeError(*line_index + 1, branch_tokens[0].column, "Solo se permite un catch por bloque try.");
            return false;
        }

        if (!ParseStatement(line_index, branch_tokens, &catch_branch, out_error)) {
            return false;
        }
    }

    if (*line_index >= lines_.size()) {
        *out_error = MakeError(*line_index, 1, "Falta 'endtry' para cerrar bloque try/catch.");
        return false;
    }

    control_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
    if (control_tokens.empty() || control_tokens[0].kind != TokenKind::KeywordEndTry) {
        const std::size_t column = control_tokens.empty() ? 1 : control_tokens[0].column;
        *out_error = MakeError(*line_index + 1, column, "Se esperaba 'endtry'.");
        return false;
    }

    if (control_tokens.size() != 1) {
        *out_error = MakeError(*line_index + 1, control_tokens[1].column, "'endtry' no acepta tokens adicionales.");
        return false;
    }

    out_statements->push_back(std::make_unique<TryCatchStmt>(
        std::move(try_branch), std::move(catch_type), std::move(error_binding), std::move(catch_branch)));
    ++(*line_index);
    return true;
}

bool Parser::ParseWhile(std::size_t* line_index, const std::vector<Token>& tokens,
                        std::vector<std::unique_ptr<Statement>>* out_statements, Diagnostic* out_error) const {

    if (tokens.size() < 3) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Instruccion while incompleta.");
        return false;
    }

    if (tokens.back().kind != TokenKind::Colon) {
        *out_error = MakeError(*line_index + 1, tokens.back().column, "Falta ':' al final del while.");
        return false;
    }

    std::vector<Token> condition_tokens(tokens.begin() + 1, tokens.end() - 1);
    std::unique_ptr<Expr> condition;
    if (!ParseExpression(*line_index + 1, std::move(condition_tokens), &condition, out_error)) {
        return false;
    }

    std::vector<std::unique_ptr<Statement>> body;

    ++(*line_index);

    if (!ParseBlock(line_index, true, &body, out_error)) {
        return false;
    }

    if (*line_index >= lines_.size()) {
        *out_error = MakeError(*line_index, 1, "Falta 'endwhile' para cerrar el bloque while.");
        return false;
    }

    std::vector<Token> control_tokens = Tokenizer::TokenizeLine(lines_[*line_index]);
    if (control_tokens.empty() || control_tokens[0].kind != TokenKind::KeywordEndWhile) {
        const std::size_t column = control_tokens.empty() ? 1 : control_tokens[0].column;
        *out_error = MakeError(*line_index + 1, column, "Se esperaba 'endwhile' para cerrar el bucle.");
        return false;
    }

    if (control_tokens.size() > 1) {
        *out_error = MakeError(*line_index + 1, control_tokens[1].column, "Tokens extra despues de 'endwhile'.");
        return false;
    }

    out_statements->push_back(std::make_unique<WhileStmt>(std::move(condition), std::move(body)));

    ++(*line_index);
    return true;
}

bool Parser::ParseMutation(std::size_t* line_index, const std::vector<Token>& tokens,
                           std::vector<std::unique_ptr<Statement>>* out_statements, Diagnostic* out_error) const {
    if (tokens.size() < 4) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Asignacion de mutacion incompleta.");
        return false;
    }

    if (tokens.back().kind != TokenKind::Semicolon) {
        *out_error = MakeError(*line_index + 1, tokens.back().column, "Falta ';' al final de la mutacion.");
        return false;
    }

    std::size_t operator_index = 0;
    AssignmentOp assignment_op = AssignmentOp::Set;
    if (!internal::FindTopLevelAssignmentOperator(tokens, &operator_index, &assignment_op)) {
        *out_error =
            MakeError(*line_index + 1, tokens[0].column, "No se encontro operador de asignacion para mutacion.");
        return false;
    }

    if (operator_index == 0 || operator_index + 1 >= tokens.size() - 1) {
        *out_error = MakeError(*line_index + 1, tokens[operator_index].column,
                               "Mutacion invalida: falta expresion en un lado de la asignacion.");
        return false;
    }

    std::vector<Token> target_tokens(tokens.begin(), tokens.begin() + static_cast<std::ptrdiff_t>(operator_index));
    std::vector<Token> value_tokens(tokens.begin() + static_cast<std::ptrdiff_t>(operator_index + 1), tokens.end() - 1);

    std::unique_ptr<Expr> target_expression;
    if (!ParseExpression(*line_index + 1, std::move(target_tokens), &target_expression, out_error)) {
        return false;
    }

    std::unique_ptr<Expr> value_expression;
    if (!ParseExpression(*line_index + 1, std::move(value_tokens), &value_expression, out_error)) {
        return false;
    }

    if (dynamic_cast<VariableExpr*>(target_expression.get()) == nullptr &&
        dynamic_cast<IndexExpr*>(target_expression.get()) == nullptr) {
        *out_error = MakeError(*line_index + 1, tokens[0].column,
                               "El lado izquierdo de una mutacion debe ser variable o indexacion.");
        return false;
    }

    out_statements->push_back(
        std::make_unique<MutationStmt>(std::move(target_expression), assignment_op, std::move(value_expression)));
    ++(*line_index);
    return true;
}

bool Parser::ParseReturn(std::size_t* line_index, const std::vector<Token>& tokens,
                         std::vector<std::unique_ptr<Statement>>* out_statements, Diagnostic* out_error) const {
    if (tokens.size() < 2 || tokens.back().kind != TokenKind::Semicolon) {
        *out_error =
            MakeError(*line_index + 1, tokens[0].column, "Formato invalido en return. Use: return; o return expr;");
        return false;
    }

    std::unique_ptr<Expr> return_expr;
    if (tokens.size() > 2) {
        std::vector<Token> expression_tokens(tokens.begin() + 1, tokens.end() - 1);
        if (!ParseExpression(*line_index + 1, std::move(expression_tokens), &return_expr, out_error)) {
            return false;
        }
    }

    out_statements->push_back(std::make_unique<ReturnStmt>(std::move(return_expr)));
    ++(*line_index);
    return true;
}

bool Parser::ParseExpressionStatement(std::size_t* line_index, const std::vector<Token>& tokens,
                                      std::vector<std::unique_ptr<Statement>>* out_statements,
                                      Diagnostic* out_error) const {
    std::vector<Token> expression_tokens = tokens;
    if (!expression_tokens.empty() && expression_tokens.back().kind == TokenKind::Semicolon) {
        expression_tokens.pop_back();
    }

    if (expression_tokens.empty()) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Sentencia de expresion vacia.");
        return false;
    }

    std::unique_ptr<Expr> expression;
    if (!ParseExpression(*line_index + 1, std::move(expression_tokens), &expression, out_error)) {
        return false;
    }

    out_statements->push_back(std::make_unique<ExpressionStmt>(std::move(expression)));
    ++(*line_index);
    return true;
}

} // namespace clot::frontend
