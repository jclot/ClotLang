#ifndef CLOT_FRONTEND_SOURCE_LOADER_HPP
#define CLOT_FRONTEND_SOURCE_LOADER_HPP

#include <string>
#include <vector>

namespace clot::frontend {

bool LoadSourceLines(
    const std::string& file_path,
    std::vector<std::string>* out_lines,
    std::string* out_error);

}  // namespace clot::frontend

#endif  // CLOT_FRONTEND_SOURCE_LOADER_HPP
