#pragma once

#include <cstdlib>
#include <optional>
#include <string>

namespace clot::runtime {

inline std::optional<std::string> GetEnvVar(const char* name) {
#ifdef _WIN32
    char* value = nullptr;
    size_t value_len = 0;
    if (_dupenv_s(&value, &value_len, name) != 0 || value == nullptr) {
        return std::nullopt;
    }
    std::string result(value);
    std::free(value);
    return result;
#else
    if (const char* value = std::getenv(name); value != nullptr) {
        return std::string(value);
    }
    return std::nullopt;
#endif
}

}  // namespace clot::runtime
