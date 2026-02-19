#ifndef CLOT_FRONTEND_TOKENIZER_HPP
#define CLOT_FRONTEND_TOKENIZER_HPP

#include <string>
#include <vector>

#include "clot/frontend/token.hpp"

namespace clot::frontend {

class Tokenizer {
public:
    static std::vector<Token> TokenizeLine(const std::string& line);
};

}  // namespace clot::frontend

#endif  // CLOT_FRONTEND_TOKENIZER_HPP
