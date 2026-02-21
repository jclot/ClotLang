#ifndef CLOT_FRONTEND_STATIC_ANALYZER_HPP
#define CLOT_FRONTEND_STATIC_ANALYZER_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "clot/frontend/ast.hpp"

namespace clot::frontend {

struct AnalysisDiagnostic {
    std::size_t line = 0;
    std::size_t column = 0;
    std::string message;
};

struct AnalysisReport {
    std::vector<AnalysisDiagnostic> errors;
    std::vector<AnalysisDiagnostic> warnings;
};

class StaticAnalyzer {
public:
    void Analyze(const Program& program, AnalysisReport* out_report) const;
};

}  // namespace clot::frontend

#endif  // CLOT_FRONTEND_STATIC_ANALYZER_HPP
