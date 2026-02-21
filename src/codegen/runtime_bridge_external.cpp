#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifdef CLOT_EXTERNAL_RUNTIME_BRIDGE_IMPL

namespace {

std::string QuoteForShell(const std::string& value) {
    std::string quoted = "'";
    for (char character : value) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted.push_back(character);
        }
    }
    quoted += "'";
    return quoted;
}

} // namespace

extern "C" int clot_runtime_execute_source(const char* source_text, const char* source_path) {

    std::filesystem::path working_dir;

    if (source_path != nullptr && source_path[0] != '\0') {
        working_dir = std::filesystem::path(source_path).parent_path();
    } else {
        working_dir = std::filesystem::current_path();
    }

    if (source_text == nullptr) {
        std::cerr << "Error de runtime bridge externo: source_text nulo.\n";
        return 1;
    }

    std::error_code ec;
    const std::filesystem::path temp_dir = std::filesystem::temp_directory_path(ec);
    std::filesystem::path base_dir;

    if (source_path != nullptr && source_path[0] != '\0') {
        base_dir = std::filesystem::path(source_path).parent_path();
    } else {
        base_dir = std::filesystem::current_path();
    }

    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::filesystem::path temp_file = base_dir / ("clot_external_bridge_" + std::to_string(now) + ".clot");

    {
        std::ofstream output(temp_file, std::ios::binary | std::ios::trunc);
        if (!output.is_open()) {
            std::cerr << "Error de runtime bridge externo: no se pudo crear archivo temporal.\n";
            return 1;
        }

        output << source_text;
        if (!output.good()) {
            std::cerr << "Error de runtime bridge externo: fallo al escribir archivo temporal.\n";
            return 1;
        }
    }

    const std::string command =
        "cd " + QuoteForShell(working_dir.string()) + " && clot " + QuoteForShell(temp_file.string());
    const int status = std::system(command.c_str());

    std::error_code ignored;
    std::filesystem::remove(temp_file, ignored);

    return status == 0 ? 0 : 1;
}

#endif // CLOT_EXTERNAL_RUNTIME_BRIDGE_IMPL
