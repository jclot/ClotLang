#include "clot/frontend/source_loader.hpp"

#include <fstream>

namespace clot::frontend {

bool LoadSourceLines(
    const std::string& file_path,
    std::vector<std::string>* out_lines,
    std::string* out_error) {
    if (out_lines == nullptr || out_error == nullptr) {
        return false;
    }

    std::ifstream input(file_path);
    if (!input.is_open()) {
        *out_error = "No se pudo abrir el archivo: " + file_path;
        return false;
    }

    out_lines->clear();

    std::string line;
    while (std::getline(input, line)) {
        out_lines->push_back(line);
    }

    if (input.bad()) {
        *out_error = "Error leyendo el archivo: " + file_path;
        return false;
    }

    return true;
}

}  // namespace clot::frontend
