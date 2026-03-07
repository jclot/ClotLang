#include "clot/frontend/tokenizer.hpp"

#include <cctype>

namespace clot::frontend {

namespace {

bool IsIdentifierStart(char character) {
    return std::isalpha(static_cast<unsigned char>(character)) || character == '_';
}

bool IsIdentifierBody(char character) {
    return std::isalnum(static_cast<unsigned char>(character)) || character == '_' || character == '.';
}

TokenKind KeywordToTokenKind(const std::string& text) {
    if (text == "print") {
        return TokenKind::KeywordPrint;
    }
    if (text == "println") {
        return TokenKind::KeywordPrintln;
    }
    if (text == "if") {
        return TokenKind::KeywordIf;
    }
    if (text == "else") {
        return TokenKind::KeywordElse;
    }
    if (text == "endif") {
        return TokenKind::KeywordEndIf;
    }
    if (text == "int") {
        return TokenKind::KeywordInt;
    }
    if (text == "double") {
        return TokenKind::KeywordDouble;
    }
    if (text == "float") {
        return TokenKind::KeywordFloat;
    }
    if (text == "decimal") {
        return TokenKind::KeywordDecimal;
    }
    if (text == "long") {
        return TokenKind::KeywordLong;
    }
    if (text == "byte") {
        return TokenKind::KeywordByte;
    }
    if (text == "char") {
        return TokenKind::KeywordChar;
    }
    if (text == "tuple") {
        return TokenKind::KeywordTuple;
    }
    if (text == "set") {
        return TokenKind::KeywordSet;
    }
    if (text == "map") {
        return TokenKind::KeywordMap;
    }
    if (text == "null") {
        return TokenKind::KeywordNull;
    }
    if (text == "enum") {
        return TokenKind::KeywordEnum;
    }
    if (text == "function") {
        return TokenKind::KeywordFunctionType;
    }
    if (text == "func") {
        return TokenKind::KeywordFunc;
    }
    if (text == "endfunc") {
        return TokenKind::KeywordEndFunc;
    }
    if (text == "import") {
        return TokenKind::KeywordImport;
    }
    if (text == "for") {
        return TokenKind::KeywordFor;
    }
    if (text == "endfor") {
        return TokenKind::KeywordEndFor;
    }
    if (text == "do") {
        return TokenKind::KeywordDo;
    }
    if (text == "switch") {
        return TokenKind::KeywordSwitch;
    }
    if (text == "case") {
        return TokenKind::KeywordCase;
    }
    if (text == "default") {
        return TokenKind::KeywordDefault;
    }
    if (text == "endswitch") {
        return TokenKind::KeywordEndSwitch;
    }
    if (text == "break") {
        return TokenKind::KeywordBreak;
    }
    if (text == "continue") {
        return TokenKind::KeywordContinue;
    }
    if (text == "pass") {
        return TokenKind::KeywordPass;
    }
    if (text == "finally") {
        return TokenKind::KeywordFinally;
    }
    if (text == "in") {
        return TokenKind::KeywordIn;
    }
    if (text == "const") {
        return TokenKind::KeywordConst;
    }
    if (text == "defer") {
        return TokenKind::KeywordDefer;
    }
    if (text == "return") {
        return TokenKind::KeywordReturn;
    }
    if (text == "try") {
        return TokenKind::KeywordTry;
    }
    if (text == "catch") {
        return TokenKind::KeywordCatch;
    }
    if (text == "endtry") {
        return TokenKind::KeywordEndTry;
    }
    if (text == "while") {
        return TokenKind::KeywordWhile;
    }
    if (text == "endwhile") {
        return TokenKind::KeywordEndWhile;
    }
    if (text == "class") {
        return TokenKind::KeywordClass;
    }
    if (text == "endclass") {
        return TokenKind::KeywordEndClass;
    }
    if (text == "interface") {
        return TokenKind::KeywordInterface;
    }
    if (text == "endinterface") {
        return TokenKind::KeywordEndInterface;
    }
    if (text == "extends") {
        return TokenKind::KeywordExtends;
    }
    if (text == "implements") {
        return TokenKind::KeywordImplements;
    }
    if (text == "constructor") {
        return TokenKind::KeywordConstructor;
    }
    if (text == "endconstructor") {
        return TokenKind::KeywordEndConstructor;
    }
    if (text == "get") {
        return TokenKind::KeywordGet;
    }
    if (text == "endget") {
        return TokenKind::KeywordEndGet;
    }
    if (text == "endset") {
        return TokenKind::KeywordEndSet;
    }
    if (text == "public") {
        return TokenKind::KeywordPublic;
    }
    if (text == "private") {
        return TokenKind::KeywordPrivate;
    }
    if (text == "static") {
        return TokenKind::KeywordStatic;
    }
    if (text == "readonly") {
        return TokenKind::KeywordReadonly;
    }
    if (text == "override") {
        return TokenKind::KeywordOverride;
    }
    if (text == "true" || text == "false") {
        return TokenKind::Boolean;
    }
    return TokenKind::Identifier;
}

}  // namespace

const char* ToString(TokenKind kind) {
    switch (kind) {
    case TokenKind::Identifier:
        return "Identifier";
    case TokenKind::Number:
        return "Number";
    case TokenKind::String:
        return "String";
    case TokenKind::Char:
        return "Char";
    case TokenKind::Boolean:
        return "Boolean";
    case TokenKind::KeywordPrint:
        return "print";
    case TokenKind::KeywordPrintln:
        return "println";
    case TokenKind::KeywordIf:
        return "if";
    case TokenKind::KeywordElse:
        return "else";
    case TokenKind::KeywordEndIf:
        return "endif";
    case TokenKind::KeywordInt:
        return "int";
    case TokenKind::KeywordDouble:
        return "double";
    case TokenKind::KeywordFloat:
        return "float";
    case TokenKind::KeywordDecimal:
        return "decimal";
    case TokenKind::KeywordLong:
        return "long";
    case TokenKind::KeywordByte:
        return "byte";
    case TokenKind::KeywordChar:
        return "char";
    case TokenKind::KeywordTuple:
        return "tuple";
    case TokenKind::KeywordSet:
        return "set";
    case TokenKind::KeywordMap:
        return "map";
    case TokenKind::KeywordNull:
        return "null";
    case TokenKind::KeywordEnum:
        return "enum";
    case TokenKind::KeywordFunctionType:
        return "function";
    case TokenKind::KeywordFunc:
        return "func";
    case TokenKind::KeywordEndFunc:
        return "endfunc";
    case TokenKind::KeywordImport:
        return "import";
    case TokenKind::KeywordFor:
        return "for";
    case TokenKind::KeywordEndFor:
        return "endfor";
    case TokenKind::KeywordDo:
        return "do";
    case TokenKind::KeywordSwitch:
        return "switch";
    case TokenKind::KeywordCase:
        return "case";
    case TokenKind::KeywordDefault:
        return "default";
    case TokenKind::KeywordEndSwitch:
        return "endswitch";
    case TokenKind::KeywordBreak:
        return "break";
    case TokenKind::KeywordContinue:
        return "continue";
    case TokenKind::KeywordPass:
        return "pass";
    case TokenKind::KeywordFinally:
        return "finally";
    case TokenKind::KeywordIn:
        return "in";
    case TokenKind::KeywordConst:
        return "const";
    case TokenKind::KeywordDefer:
        return "defer";
    case TokenKind::KeywordReturn:
        return "return";
    case TokenKind::KeywordTry:
        return "try";
    case TokenKind::KeywordCatch:
        return "catch";
    case TokenKind::KeywordEndTry:
        return "endtry";
    case TokenKind::KeywordWhile:
        return "while";
    case TokenKind::KeywordEndWhile:
        return "endwhile";
    case TokenKind::KeywordClass:
        return "class";
    case TokenKind::KeywordEndClass:
        return "endclass";
    case TokenKind::KeywordInterface:
        return "interface";
    case TokenKind::KeywordEndInterface:
        return "endinterface";
    case TokenKind::KeywordExtends:
        return "extends";
    case TokenKind::KeywordImplements:
        return "implements";
    case TokenKind::KeywordConstructor:
        return "constructor";
    case TokenKind::KeywordEndConstructor:
        return "endconstructor";
    case TokenKind::KeywordGet:
        return "get";
    case TokenKind::KeywordEndGet:
        return "endget";
    case TokenKind::KeywordEndSet:
        return "endset";
    case TokenKind::KeywordPublic:
        return "public";
    case TokenKind::KeywordPrivate:
        return "private";
    case TokenKind::KeywordStatic:
        return "static";
    case TokenKind::KeywordReadonly:
        return "readonly";
    case TokenKind::KeywordOverride:
        return "override";
    case TokenKind::Assign:
        return "=";
    case TokenKind::PlusEqual:
        return "+=";
    case TokenKind::MinusEqual:
        return "-=";
    case TokenKind::Plus:
        return "+";
    case TokenKind::Minus:
        return "-";
    case TokenKind::Star:
        return "*";
    case TokenKind::Slash:
        return "/";
    case TokenKind::Percent:
        return "%";
    case TokenKind::Caret:
        return "^";
    case TokenKind::And:
        return "&&";
    case TokenKind::Or:
        return "||";
    case TokenKind::Not:
        return "!";
    case TokenKind::EqualEqual:
        return "==";
    case TokenKind::BangEqual:
        return "!=";
    case TokenKind::Less:
        return "<";
    case TokenKind::LessEqual:
        return "<=";
    case TokenKind::Greater:
        return ">";
    case TokenKind::GreaterEqual:
        return ">=";
    case TokenKind::LeftParen:
        return "(";
    case TokenKind::RightParen:
        return ")";
    case TokenKind::LeftBracket:
        return "[";
    case TokenKind::RightBracket:
        return "]";
    case TokenKind::LeftBrace:
        return "{";
    case TokenKind::RightBrace:
        return "}";
    case TokenKind::Comma:
        return ",";
    case TokenKind::Colon:
        return ":";
    case TokenKind::Semicolon:
        return ";";
    case TokenKind::Ampersand:
        return "&";
    case TokenKind::Unknown:
    default:
        return "Unknown";
    }
}

std::vector<Token> Tokenizer::TokenizeLine(const std::string& line) {
    std::vector<Token> tokens;

    std::size_t index = 0;
    while (index < line.size()) {
        const char current = line[index];

        if (std::isspace(static_cast<unsigned char>(current))) {
            ++index;
            continue;
        }

        if (current == '/' && index + 1 < line.size() && line[index + 1] == '/') {
            break;
        }

        if (current == '#') {
            break;
        }

        if (current == '"') {
            std::size_t cursor = index + 1;
            bool escaped = false;
            std::string literal;

            while (cursor < line.size()) {
                const char candidate = line[cursor];
                if (!escaped && candidate == '"') {
                    break;
                }

                if (!escaped && candidate == '\\') {
                    escaped = true;
                } else {
                    literal.push_back(candidate);
                    escaped = false;
                }
                ++cursor;
            }

            if (cursor >= line.size()) {
                tokens.push_back({TokenKind::Unknown, line.substr(index), index + 1});
                break;
            }

            tokens.push_back({TokenKind::String, literal, index + 1});
            index = cursor + 1;
            continue;
        }

        if (current == '\'') {
            std::size_t cursor = index + 1;
            if (cursor >= line.size()) {
                tokens.push_back({TokenKind::Unknown, line.substr(index), index + 1});
                break;
            }

            char parsed = '\0';
            if (line[cursor] == '\\') {
                ++cursor;
                if (cursor >= line.size()) {
                    tokens.push_back({TokenKind::Unknown, line.substr(index), index + 1});
                    break;
                }

                const char escaped = line[cursor];
                if (escaped == 'n') {
                    parsed = '\n';
                } else if (escaped == 't') {
                    parsed = '\t';
                } else if (escaped == 'r') {
                    parsed = '\r';
                } else if (escaped == '\'') {
                    parsed = '\'';
                } else if (escaped == '\\') {
                    parsed = '\\';
                } else if (escaped == '0') {
                    parsed = '\0';
                } else {
                    tokens.push_back({TokenKind::Unknown, line.substr(index, cursor - index + 1), index + 1});
                    break;
                }
                ++cursor;
            } else {
                parsed = line[cursor];
                ++cursor;
            }

            if (cursor >= line.size() || line[cursor] != '\'') {
                tokens.push_back({TokenKind::Unknown, line.substr(index), index + 1});
                break;
            }

            tokens.push_back({TokenKind::Char, std::string(1, parsed), index + 1});
            index = cursor + 1;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(current)) ||
            (current == '.' && index + 1 < line.size() && std::isdigit(static_cast<unsigned char>(line[index + 1])))) {
            std::size_t cursor = index;
            bool has_dot = false;

            while (cursor < line.size()) {
                const char candidate = line[cursor];
                if (candidate == '.') {
                    if (has_dot) {
                        break;
                    }
                    has_dot = true;
                    ++cursor;
                    continue;
                }
                if (!std::isdigit(static_cast<unsigned char>(candidate))) {
                    break;
                }
                ++cursor;
            }

            tokens.push_back({TokenKind::Number, line.substr(index, cursor - index), index + 1});
            index = cursor;
            continue;
        }

        if (IsIdentifierStart(current)) {
            std::size_t cursor = index + 1;
            while (cursor < line.size() && IsIdentifierBody(line[cursor])) {
                ++cursor;
            }

            const std::string text = line.substr(index, cursor - index);
            tokens.push_back({KeywordToTokenKind(text), text, index + 1});
            index = cursor;
            continue;
        }

        if (index + 1 < line.size()) {
            const std::string two_char = line.substr(index, 2);
            if (two_char == "==") {
                tokens.push_back({TokenKind::EqualEqual, two_char, index + 1});
                index += 2;
                continue;
            }
            if (two_char == "!=") {
                tokens.push_back({TokenKind::BangEqual, two_char, index + 1});
                index += 2;
                continue;
            }
            if (two_char == "<=") {
                tokens.push_back({TokenKind::LessEqual, two_char, index + 1});
                index += 2;
                continue;
            }
            if (two_char == ">=") {
                tokens.push_back({TokenKind::GreaterEqual, two_char, index + 1});
                index += 2;
                continue;
            }
            if (two_char == "&&") {
                tokens.push_back({TokenKind::And, two_char, index + 1});
                index += 2;
                continue;
            }
            if (two_char == "||") {
                tokens.push_back({TokenKind::Or, two_char, index + 1});
                index += 2;
                continue;
            }
            if (two_char == "+=") {
                tokens.push_back({TokenKind::PlusEqual, two_char, index + 1});
                index += 2;
                continue;
            }
            if (two_char == "-=") {
                tokens.push_back({TokenKind::MinusEqual, two_char, index + 1});
                index += 2;
                continue;
            }
        }

        Token token;
        token.lexeme = std::string(1, current);
        token.column = index + 1;

        switch (current) {
        case '=':
            token.kind = TokenKind::Assign;
            break;
        case '+':
            token.kind = TokenKind::Plus;
            break;
        case '-':
            token.kind = TokenKind::Minus;
            break;
        case '*':
            token.kind = TokenKind::Star;
            break;
        case '/':
            token.kind = TokenKind::Slash;
            break;
        case '%':
            token.kind = TokenKind::Percent;
            break;
        case '^':
            token.kind = TokenKind::Caret;
            break;
        case '!':
            token.kind = TokenKind::Not;
            break;
        case '<':
            token.kind = TokenKind::Less;
            break;
        case '>':
            token.kind = TokenKind::Greater;
            break;
        case '(':
            token.kind = TokenKind::LeftParen;
            break;
        case ')':
            token.kind = TokenKind::RightParen;
            break;
        case '[':
            token.kind = TokenKind::LeftBracket;
            break;
        case ']':
            token.kind = TokenKind::RightBracket;
            break;
        case '{':
            token.kind = TokenKind::LeftBrace;
            break;
        case '}':
            token.kind = TokenKind::RightBrace;
            break;
        case ',':
            token.kind = TokenKind::Comma;
            break;
        case ':':
            token.kind = TokenKind::Colon;
            break;
        case ';':
            token.kind = TokenKind::Semicolon;
            break;
        case '&':
            token.kind = TokenKind::Ampersand;
            break;
        default:
            token.kind = TokenKind::Unknown;
            break;
        }

        tokens.push_back(token);
        ++index;
    }

    return tokens;
}

}  // namespace clot::frontend
