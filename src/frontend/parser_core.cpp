#include "clot/frontend/parser.hpp"

#include <memory>
#include <utility>
#include <vector>

#include "clot/frontend/tokenizer.hpp"
#include "parser_support.hpp"

namespace clot::frontend {

Parser::Parser(std::vector<std::string> lines) : lines_(std::move(lines)) {}

bool Parser::Parse(Program* out_program, Diagnostic* out_error) const {
    if (out_program == nullptr || out_error == nullptr) {
        return false;
    }

    out_program->statements.clear();
    out_error->line = 0;
    out_error->column = 0;
    out_error->message.clear();

    std::size_t line_index = 0;
    return ParseBlock(&line_index, false, &out_program->statements, out_error);
}

bool Parser::ParseBlock(
    std::size_t* line_index,
    bool stop_at_control_token,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
    while (*line_index < lines_.size()) {
        const std::vector<Token> tokens = Tokenizer::TokenizeLine(lines_[*line_index]);

        if (tokens.empty()) {
            ++(*line_index);
            continue;
        }

        if (tokens[0].kind == TokenKind::Unknown) {
            *out_error = MakeError(*line_index + 1, tokens[0].column, "Token no reconocido: '" + tokens[0].lexeme + "'.");
            return false;
        }

        if (stop_at_control_token && internal::IsControlToken(tokens)) {
            return true;
        }

        if (!ParseStatement(line_index, tokens, out_statements, out_error)) {
            return false;
        }
    }

    return true;
}

bool Parser::ParseStatement(
    std::size_t* line_index,
    const std::vector<Token>& tokens,
    std::vector<std::unique_ptr<Statement>>* out_statements,
    Diagnostic* out_error) const {
    if (tokens[0].kind == TokenKind::KeywordPrint) {
        return ParsePrint(line_index, tokens, out_statements, out_error);
    }

    if (tokens[0].kind == TokenKind::KeywordIf) {
        return ParseIf(line_index, tokens, out_statements, out_error);
    }

    if (tokens[0].kind == TokenKind::KeywordFunc) {
        return ParseFunctionDeclaration(line_index, tokens, out_statements, out_error);
    }

    if (tokens[0].kind == TokenKind::KeywordImport) {
        return ParseImport(line_index, tokens, out_statements, out_error);
    }

    if (tokens[0].kind == TokenKind::KeywordReturn) {
        return ParseReturn(line_index, tokens, out_statements, out_error);
    }

    if (tokens[0].kind == TokenKind::KeywordElse ||
        tokens[0].kind == TokenKind::KeywordEndIf ||
        tokens[0].kind == TokenKind::KeywordEndFunc) {
        *out_error = MakeError(
            *line_index + 1,
            tokens[0].column,
            "Token de control fuera de bloque: '" + tokens[0].lexeme + "'.");
        return false;
    }

    if (tokens[0].kind == TokenKind::KeywordLong || tokens[0].kind == TokenKind::KeywordByte) {
        return ParseAssignment(line_index, tokens, out_statements, out_error);
    }

    if (tokens[0].kind == TokenKind::Identifier &&
        tokens.size() > 1 &&
        (tokens[1].kind == TokenKind::Assign ||
         tokens[1].kind == TokenKind::PlusEqual ||
         tokens[1].kind == TokenKind::MinusEqual)) {
        return ParseAssignment(line_index, tokens, out_statements, out_error);
    }

    std::size_t mutation_operator_index = 0;
    AssignmentOp mutation_op = AssignmentOp::Set;
    if (internal::FindTopLevelAssignmentOperator(tokens, &mutation_operator_index, &mutation_op)) {
        (void)mutation_operator_index;
        (void)mutation_op;
        return ParseMutation(line_index, tokens, out_statements, out_error);
    }

    return ParseExpressionStatement(line_index, tokens, out_statements, out_error);
}

Diagnostic Parser::MakeError(
    std::size_t line,
    std::size_t column,
    const std::string& message) const {
    return Diagnostic{line, column, message};
}

}  // namespace clot::frontend
