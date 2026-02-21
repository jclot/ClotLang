#include "clot/frontend/parser.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "clot/frontend/tokenizer.hpp"
#include "parser_support.hpp"

namespace clot::frontend {

bool Parser::ParseAssignment(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
    DeclarationType declaration_type = DeclarationType::Inferred;
    std::size_t cursor = 0;

    if (tokens[cursor].kind == TokenKind::KeywordLong) {
        declaration_type = DeclarationType::Long;
        ++cursor;
    } else if (tokens[cursor].kind == TokenKind::KeywordByte) {
        declaration_type = DeclarationType::Byte;
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

    out_statements->push_back(std::make_unique<AssignmentStmt>(
        variable_name,
        assignment_op,
        declaration_type,
        std::move(expression)));

    ++(*line_index);
    return true;
}

bool Parser::ParsePrint(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
    if (tokens.size() < 5) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Instruccion print incompleta.");
        return false;
    }

    if (tokens[1].kind != TokenKind::LeftParen) {
        *out_error = MakeError(*line_index + 1, tokens[1].column, "Se esperaba '(' despues de print.");
        return false;
    }

    if (tokens.back().kind != TokenKind::Semicolon) {
        *out_error = MakeError(*line_index + 1, tokens.back().column, "Falta ';' al final de print.");
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
        *out_error = MakeError(*line_index + 1, tokens.back().column, "print requiere cerrar ')' antes de ';'.");
        return false;
    }

    if (closing_paren_index <= 2) {
        *out_error = MakeError(*line_index + 1, tokens[1].column, "print requiere una expresion interna.");
        return false;
    }

    std::vector<Token> expression_tokens(tokens.begin() + 2, tokens.begin() + static_cast<std::ptrdiff_t>(closing_paren_index));

    std::unique_ptr<Expr> expression;
    if (!ParseExpression(*line_index + 1, std::move(expression_tokens), &expression, out_error)) {
        return false;
    }

    out_statements->push_back(std::make_unique<PrintStmt>(std::move(expression)));
    ++(*line_index);
    return true;
}

bool Parser::ParseIf(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
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

bool Parser::ParseFunctionDeclaration(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
    if (tokens.size() < 6) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Declaracion de funcion incompleta.");
        return false;
    }

    if (tokens[1].kind != TokenKind::Identifier) {
        *out_error = MakeError(*line_index + 1, tokens[1].column, "Falta nombre de funcion valido.");
        return false;
    }

    if (tokens[2].kind != TokenKind::LeftParen) {
        *out_error = MakeError(*line_index + 1, tokens[2].column, "Se esperaba '(' en la declaracion de funcion.");
        return false;
    }

    if (tokens.back().kind != TokenKind::Colon) {
        *out_error = MakeError(*line_index + 1, tokens.back().column, "Falta ':' al final de la declaracion de funcion.");
        return false;
    }

    const std::string function_name = tokens[1].lexeme;
    std::vector<FunctionParam> params;

    std::size_t cursor = 3;
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
            *out_error = MakeError(*line_index + 1, tokens[error_cursor].column, "Parametro invalido en declaracion de funcion.");
            return false;
        }

        params.push_back(FunctionParam{tokens[cursor].lexeme, by_reference});
        ++cursor;

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

        *out_error = MakeError(*line_index + 1, tokens[cursor].column, "Se esperaba ',' o ')' en parametros de funcion.");
        return false;
    }

    if (cursor >= tokens.size() || tokens[cursor].kind != TokenKind::Colon) {
        const std::size_t error_cursor = cursor < tokens.size() ? cursor : tokens.size() - 1;
        *out_error = MakeError(*line_index + 1, tokens[error_cursor].column, "Declaracion de funcion invalida: falta ':' final.");
        return false;
    }

    if (cursor != tokens.size() - 1) {
        *out_error = MakeError(*line_index + 1, tokens[cursor + 1].column, "Tokens extra despues de declaracion de funcion.");
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
                *out_error = MakeError(*line_index + 1, body_tokens[1].column, "'endfunc' no acepta tokens adicionales.");
                return false;
            }

            out_statements->push_back(std::make_unique<FunctionDeclStmt>(function_name, std::move(params), std::move(body)));
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

bool Parser::ParseImport(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
    if (tokens.size() != 3 || tokens[1].kind != TokenKind::Identifier || tokens[2].kind != TokenKind::Semicolon) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Formato invalido en import. Use: import modulo;");
        return false;
    }

    out_statements->push_back(std::make_unique<ImportStmt>(tokens[1].lexeme));
    ++(*line_index);
    return true;
}

bool Parser::ParseTry(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
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
            *out_error = MakeError(
                *line_index + 1,
                branch_tokens[0].column,
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

    std::string error_binding;
    if (control_tokens.size() == 2) {
        // catch:
    } else if (
        control_tokens.size() == 5 &&
        control_tokens[1].kind == TokenKind::LeftParen &&
        control_tokens[2].kind == TokenKind::Identifier &&
        control_tokens[3].kind == TokenKind::RightParen) {
        error_binding = control_tokens[2].lexeme;
    } else {
        *out_error = MakeError(
            *line_index + 1,
            control_tokens[0].column,
            "Formato invalido en catch. Use: catch: o catch(error):");
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
            *out_error = MakeError(
                *line_index + 1,
                branch_tokens[0].column,
                "Token no reconocido: '" + branch_tokens[0].lexeme + "'.");
            return false;
        }

        if (branch_tokens[0].kind == TokenKind::KeywordEndTry) {
            break;
        }

        if (branch_tokens[0].kind == TokenKind::KeywordCatch) {
            *out_error = MakeError(*line_index + 1, branch_tokens[0].column, "Solo se permite un catch por bloque try.");
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
        std::move(try_branch),
        std::move(error_binding),
        std::move(catch_branch)));
    ++(*line_index);
    return true;
}

bool Parser::ParseMutation(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
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
        *out_error = MakeError(*line_index + 1, tokens[0].column, "No se encontro operador de asignacion para mutacion.");
        return false;
    }

    if (operator_index == 0 || operator_index + 1 >= tokens.size() - 1) {
        *out_error = MakeError(*line_index + 1, tokens[operator_index].column, "Mutacion invalida: falta expresion en un lado de la asignacion.");
        return false;
    }

    std::vector<Token> target_tokens(tokens.begin(), tokens.begin() + static_cast<std::ptrdiff_t>(operator_index));
    std::vector<Token> value_tokens(
        tokens.begin() + static_cast<std::ptrdiff_t>(operator_index + 1),
        tokens.end() - 1);

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
        *out_error = MakeError(
            *line_index + 1,
            tokens[0].column,
            "El lado izquierdo de una mutacion debe ser variable o indexacion.");
        return false;
    }

    out_statements->push_back(std::make_unique<MutationStmt>(
        std::move(target_expression),
        assignment_op,
        std::move(value_expression)));
    ++(*line_index);
    return true;
}

bool Parser::ParseReturn(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
    if (tokens.size() < 2 || tokens.back().kind != TokenKind::Semicolon) {
        *out_error = MakeError(*line_index + 1, tokens[0].column, "Formato invalido en return. Use: return; o return expr;");
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

bool Parser::ParseExpressionStatement(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
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

}  // namespace clot::frontend
