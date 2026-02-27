#ifndef CLOT_FRONTEND_PARSER_HPP
#define CLOT_FRONTEND_PARSER_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "clot/frontend/ast.hpp"
#include "clot/frontend/token.hpp"

namespace clot::frontend {

struct Diagnostic {
    std::size_t line = 0;
    std::size_t column = 0;
    std::string message;
};

class Parser {
public:
    explicit Parser(std::vector<std::string> lines);

    bool Parse(Program* out_program, Diagnostic* out_error) const;

private:
    bool ParseBlock(
        std::size_t* line_index,
        bool stop_at_control_token,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseStatement(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseAssignment(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParsePrint(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseIf(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseFunctionDeclaration(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseImport(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseTry(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseWhile(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseMutation(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseReturn(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseExpressionStatement(
        std::size_t* line_index,
        const std::vector<Token>& tokens,
        std::vector<std::unique_ptr<Statement>>* out_statements,
        Diagnostic* out_error) const;

    bool ParseExpression(
        std::size_t line_number,
        std::vector<Token> tokens,
        std::unique_ptr<Expr>* out_expr,
        Diagnostic* out_error) const;

    Diagnostic MakeError(
        std::size_t line,
        std::size_t column,
        const std::string& message) const;

    std::vector<std::string> lines_;
};

}  // namespace clot::frontend

#endif  // CLOT_FRONTEND_PARSER_HPP
