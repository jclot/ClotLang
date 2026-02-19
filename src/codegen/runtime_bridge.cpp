#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>

#include "clot/frontend/parser.hpp"
#include "clot/interpreter/interpreter.hpp"
#include "clot/runtime/i18n.hpp"

extern "C" int clot_runtime_execute_source(const char* source_text, const char* source_path) {
    if (source_text == nullptr) {
        std::cerr << clot::runtime::TranslateDiagnostic("Error de runtime bridge: source_text nulo.") << "\n";
        return 1;
    }

    if (const char* env_lang = std::getenv("CLOT_LANG"); env_lang != nullptr) {
        clot::runtime::Language lang = clot::runtime::Language::Spanish;
        if (clot::runtime::ParseLanguage(env_lang, &lang)) {
            clot::runtime::SetLanguage(lang);
        }
    }

    std::vector<std::string> lines;
    std::istringstream stream(source_text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    // Preserve trailing empty line when input ends with '\n'.
    const std::string raw_text(source_text);
    if (!raw_text.empty() && raw_text.back() == '\n') {
        lines.push_back(std::string());
    }

    clot::frontend::Parser parser(std::move(lines));
    clot::frontend::Program program;
    clot::frontend::Diagnostic diagnostic;
    if (!parser.Parse(&program, &diagnostic)) {
        std::cerr << clot::runtime::Tr("Error de parseo en linea ", "Parse error at line ")
                  << diagnostic.line
                  << clot::runtime::Tr(", columna ", ", column ")
                  << diagnostic.column
                  << ": "
                  << clot::runtime::TranslateDiagnostic(diagnostic.message)
                  << "\n";
        return 1;
    }

    clot::interpreter::Interpreter interpreter;
    if (source_path != nullptr && source_path[0] != '\0') {
        interpreter.SetEntryFilePath(source_path);
    }
    std::string runtime_error;
    if (!interpreter.Execute(program, &runtime_error)) {
        std::cerr << clot::runtime::Tr("Error de ejecucion: ", "Runtime error: ")
                  << clot::runtime::TranslateDiagnostic(runtime_error) << "\n";
        return 1;
    }

    return 0;
}
