#ifndef CLOT_FRONTEND_PARSER_SUPPORT_HPP
#define CLOT_FRONTEND_PARSER_SUPPORT_HPP

#include <cstddef>
#include <vector>

#include "clot/frontend/ast.hpp"
#include "clot/frontend/token.hpp"

namespace clot::frontend::internal {

inline bool IsControlToken(const std::vector<Token>& tokens) {
    if (tokens.empty()) {
        return false;
    }

    return tokens[0].kind == TokenKind::KeywordElse || tokens[0].kind == TokenKind::KeywordEndIf;
}

inline bool TokenToAssignmentOp(TokenKind token_kind, AssignmentOp* out_op) {
    if (out_op == nullptr) {
        return false;
    }

    if (token_kind == TokenKind::Assign) {
        *out_op = AssignmentOp::Set;
        return true;
    }

    if (token_kind == TokenKind::PlusEqual) {
        *out_op = AssignmentOp::AddAssign;
        return true;
    }

    if (token_kind == TokenKind::MinusEqual) {
        *out_op = AssignmentOp::SubAssign;
        return true;
    }

    return false;
}

inline bool FindTopLevelAssignmentOperator(
    const std::vector<Token>& tokens,
    std::size_t* out_operator_index,
    AssignmentOp* out_op) {
    if (tokens.empty()) {
        return false;
    }

    int paren_depth = 0;
    int bracket_depth = 0;
    int brace_depth = 0;

    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const TokenKind kind = tokens[i].kind;
        if (kind == TokenKind::LeftParen) {
            ++paren_depth;
            continue;
        }
        if (kind == TokenKind::RightParen) {
            --paren_depth;
            continue;
        }
        if (kind == TokenKind::LeftBracket) {
            ++bracket_depth;
            continue;
        }
        if (kind == TokenKind::RightBracket) {
            --bracket_depth;
            continue;
        }
        if (kind == TokenKind::LeftBrace) {
            ++brace_depth;
            continue;
        }
        if (kind == TokenKind::RightBrace) {
            --brace_depth;
            continue;
        }

        if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0) {
            AssignmentOp op = AssignmentOp::Set;
            if (TokenToAssignmentOp(kind, &op)) {
                if (out_operator_index != nullptr) {
                    *out_operator_index = i;
                }
                if (out_op != nullptr) {
                    *out_op = op;
                }
                return true;
            }
        }
    }

    return false;
}

}  // namespace clot::frontend::internal

#endif  // CLOT_FRONTEND_PARSER_SUPPORT_HPP
