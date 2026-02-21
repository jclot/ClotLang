#ifndef CLOT_FRONTEND_TOKEN_HPP
#define CLOT_FRONTEND_TOKEN_HPP

#include <cstddef>
#include <string>

namespace clot::frontend {

enum class TokenKind {
    Identifier,
    Number,
    String,
    Boolean,

    KeywordPrint,
    KeywordIf,
    KeywordElse,
    KeywordEndIf,
    KeywordLong,
    KeywordByte,
    KeywordFunc,
    KeywordEndFunc,
    KeywordImport,
    KeywordReturn,
    KeywordTry,
    KeywordCatch,
    KeywordEndTry,

    Assign,
    PlusEqual,
    MinusEqual,
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Caret,

    And,
    Or,
    Not,

    EqualEqual,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Comma,
    Colon,
    Semicolon,
    Ampersand,

    Unknown,
};

struct Token {
    TokenKind kind = TokenKind::Unknown;
    std::string lexeme;
    std::size_t column = 0;
};

const char* ToString(TokenKind kind);

}  // namespace clot::frontend

#endif  // CLOT_FRONTEND_TOKEN_HPP
