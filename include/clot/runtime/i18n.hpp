#ifndef CLOT_RUNTIME_I18N_HPP
#define CLOT_RUNTIME_I18N_HPP

#include <string>

namespace clot::runtime {

enum class Language {
    Spanish,
    English,
};

bool ParseLanguage(const std::string& text, Language* out_language);

void SetLanguage(Language language);
Language GetLanguage();

std::string LanguageCode(Language language);

std::string Tr(const std::string& spanish, const std::string& english);

// Translate diagnostics that are currently generated mostly in Spanish.
std::string TranslateDiagnostic(const std::string& text);

}  // namespace clot::runtime

#endif  // CLOT_RUNTIME_I18N_HPP
