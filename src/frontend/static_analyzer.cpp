#include "clot/frontend/static_analyzer.hpp"

#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace clot::frontend {

namespace {

enum class TypeHint {
    Unknown,
    Number,
    String,
    Bool,
    List,
    Object,
};

struct SymbolInfo {
    DeclarationType declaration_type = DeclarationType::Inferred;
    TypeHint hint = TypeHint::Unknown;
};

struct FunctionInfo {
    std::vector<bool> by_reference_params;
};

struct ExpressionFacts {
    TypeHint hint = TypeHint::Unknown;
    bool is_constant_numeric = false;
    double constant_numeric = 0.0;
};

using SymbolTable = std::unordered_map<std::string, SymbolInfo>;
using FunctionTable = std::unordered_map<std::string, FunctionInfo>;

class AnalyzerEngine {
public:
    explicit AnalyzerEngine(AnalysisReport* report) : report_(report) {}

    void Analyze(const Program& program) {
        CollectFunctionsAndImports(program);

        SymbolTable symbols;
        AnalyzeStatements(program.statements, &symbols);
    }

private:
    void CollectFunctionsAndImports(const Program& program) {
        for (const auto& statement : program.statements) {
            CollectFunctionsAndImportsInStatement(statement.get());
        }
    }

    void CollectFunctionsAndImportsInStatement(const Statement* statement) {
        if (statement == nullptr) {
            return;
        }

        if (const auto* function_decl = dynamic_cast<const FunctionDeclStmt*>(statement)) {
            FunctionInfo info;
            info.by_reference_params.reserve(function_decl->params.size());
            for (const auto& param : function_decl->params) {
                info.by_reference_params.push_back(param.by_reference);
            }
            functions_[function_decl->name] = std::move(info);

            for (const auto& nested : function_decl->body) {
                CollectFunctionsAndImportsInStatement(nested.get());
            }
            return;
        }

        if (const auto* import_stmt = dynamic_cast<const ImportStmt*>(statement)) {
            if (import_stmt->module_name == "math") {
                math_imported_ = true;
            }
            return;
        }

        if (const auto* conditional = dynamic_cast<const IfStmt*>(statement)) {
            for (const auto& nested : conditional->then_branch) {
                CollectFunctionsAndImportsInStatement(nested.get());
            }
            for (const auto& nested : conditional->else_branch) {
                CollectFunctionsAndImportsInStatement(nested.get());
            }
            return;
        }

        if (const auto* while_stmt = dynamic_cast<const WhileStmt*>(statement)) {
            for (const auto& nested : while_stmt->body) {
                CollectFunctionsAndImportsInStatement(nested.get());
            }
            return;
        }

        if (const auto* try_catch = dynamic_cast<const TryCatchStmt*>(statement)) {
            for (const auto& nested : try_catch->try_branch) {
                CollectFunctionsAndImportsInStatement(nested.get());
            }
            for (const auto& nested : try_catch->catch_branch) {
                CollectFunctionsAndImportsInStatement(nested.get());
            }
        }
    }

    void AnalyzeStatements(
        const std::vector<std::unique_ptr<Statement>>& statements,
        SymbolTable* symbols) {
        for (const auto& statement : statements) {
            if (statement == nullptr) {
                continue;
            }
            AnalyzeStatement(*statement, symbols);
        }
    }

    void AnalyzeStatement(const Statement& statement, SymbolTable* symbols) {
        const std::size_t statement_id = next_statement_id_++;

        if (const auto* assignment = dynamic_cast<const AssignmentStmt*>(&statement)) {
            AnalyzeAssignment(*assignment, statement_id, symbols);
            return;
        }

        if (const auto* mutation = dynamic_cast<const MutationStmt*>(&statement)) {
            AnalyzeMutation(*mutation, statement_id, symbols);
            return;
        }

        if (const auto* print = dynamic_cast<const PrintStmt*>(&statement)) {
            if (print->expr != nullptr) {
                (void)InferExpression(*print->expr, statement_id, *symbols);
            }
            return;
        }

        if (const auto* while_stmt = dynamic_cast<const WhileStmt*>(&statement)) {
            (void)InferExpression(*while_stmt->condition, statement_id, *symbols);

            SymbolTable loop_symbols = *symbols;
            AnalyzeStatements(while_stmt->body, &loop_symbols);
            return;
        }

        if (const auto* conditional = dynamic_cast<const IfStmt*>(&statement)) {
            (void)InferExpression(*conditional->condition, statement_id, *symbols);

            SymbolTable then_symbols = *symbols;
            AnalyzeStatements(conditional->then_branch, &then_symbols);

            SymbolTable else_symbols = *symbols;
            AnalyzeStatements(conditional->else_branch, &else_symbols);
            return;
        }

        if (const auto* function_decl = dynamic_cast<const FunctionDeclStmt*>(&statement)) {
            SymbolTable function_symbols = *symbols;
            for (const auto& param : function_decl->params) {
                function_symbols[param.name] = SymbolInfo{DeclarationType::Inferred, TypeHint::Unknown};
            }
            AnalyzeStatements(function_decl->body, &function_symbols);
            return;
        }

        if (const auto* import_stmt = dynamic_cast<const ImportStmt*>(&statement)) {
            if (import_stmt->module_name == "math") {
                math_imported_ = true;
            }
            return;
        }

        if (const auto* expression_stmt = dynamic_cast<const ExpressionStmt*>(&statement)) {
            (void)InferExpression(*expression_stmt->expr, statement_id, *symbols);
            return;
        }

        if (const auto* return_stmt = dynamic_cast<const ReturnStmt*>(&statement)) {
            if (return_stmt->expr != nullptr) {
                (void)InferExpression(*return_stmt->expr, statement_id, *symbols);
            }
            return;
        }

        if (const auto* try_catch = dynamic_cast<const TryCatchStmt*>(&statement)) {
            SymbolTable try_symbols = *symbols;
            AnalyzeStatements(try_catch->try_branch, &try_symbols);

            SymbolTable catch_symbols = *symbols;
            if (!try_catch->error_binding.empty()) {
                catch_symbols[try_catch->error_binding] =
                    SymbolInfo{DeclarationType::Inferred, TypeHint::String};
            }
            AnalyzeStatements(try_catch->catch_branch, &catch_symbols);
            return;
        }
    }

    void AnalyzeAssignment(
        const AssignmentStmt& assignment,
        std::size_t statement_id,
        SymbolTable* symbols) {
        const ExpressionFacts rhs = InferExpression(*assignment.expr, statement_id, *symbols);

        const std::size_t dot = assignment.name.find('.');
        if (dot != std::string::npos) {
            const std::string root = assignment.name.substr(0, dot);
            if (symbols->find(root) == symbols->end()) {
                AddError(statement_id, "La propiedad '" + assignment.name + "' usa raiz no definida: '" + root + "'.");
            }
            return;
        }

        const auto found = symbols->find(assignment.name);
        const bool exists = found != symbols->end();
        if (!exists && assignment.op != AssignmentOp::Set) {
            AddError(statement_id, "Asignacion compuesta sobre variable no definida: '" + assignment.name + "'.");
            return;
        }

        DeclarationType effective_type = assignment.declaration_type;
        TypeHint current_hint = TypeHint::Unknown;
        if (exists) {
            if (effective_type == DeclarationType::Inferred) {
                effective_type = found->second.declaration_type;
            }
            current_hint = found->second.hint;
        }

        if (assignment.op == AssignmentOp::AddAssign || assignment.op == AssignmentOp::SubAssign) {
            if (current_hint == TypeHint::String && assignment.op == AssignmentOp::SubAssign) {
                AddError(statement_id, "No se puede usar '-=' sobre string en '" + assignment.name + "'.");
            }
        }

        ValidateTypedRangeIfNeeded(effective_type, rhs, statement_id, assignment.name);

        SymbolInfo updated;
        updated.declaration_type = effective_type;
        if (effective_type == DeclarationType::Long || effective_type == DeclarationType::Byte) {
            updated.hint = TypeHint::Number;
        } else if (assignment.op == AssignmentOp::AddAssign || assignment.op == AssignmentOp::SubAssign) {
            updated.hint = TypeHint::Number;
        } else {
            updated.hint = rhs.hint;
        }

        (*symbols)[assignment.name] = updated;
    }

    void AnalyzeMutation(
        const MutationStmt& mutation,
        std::size_t statement_id,
        SymbolTable* symbols) {
        const ExpressionFacts rhs = InferExpression(*mutation.expr, statement_id, *symbols);
        const std::optional<std::string> target_root = ResolveTargetRoot(*mutation.target);
        if (!target_root.has_value()) {
            AddError(statement_id, "Objetivo invalido para mutacion.");
            return;
        }

        const auto found = symbols->find(*target_root);
        if (found == symbols->end()) {
            AddError(statement_id, "Mutacion sobre variable no definida: '" + *target_root + "'.");
            return;
        }

        ValidateTypedRangeIfNeeded(found->second.declaration_type, rhs, statement_id, *target_root);
    }

    std::optional<std::string> ResolveTargetRoot(const Expr& target) {
        if (const auto* variable = dynamic_cast<const VariableExpr*>(&target)) {
            const std::size_t dot = variable->name.find('.');
            if (dot == std::string::npos) {
                return variable->name;
            }
            return variable->name.substr(0, dot);
        }

        if (const auto* index = dynamic_cast<const IndexExpr*>(&target)) {
            return ResolveTargetRoot(*index->collection);
        }

        return std::nullopt;
    }

    ExpressionFacts InferExpression(
        const Expr& expression,
        std::size_t statement_id,
        const SymbolTable& symbols) {
        if (const auto* number = dynamic_cast<const NumberExpr*>(&expression)) {
            return ExpressionFacts{TypeHint::Number, true, number->value};
        }

        if (const auto* text = dynamic_cast<const StringExpr*>(&expression)) {
            (void)text;
            return ExpressionFacts{TypeHint::String, false, 0.0};
        }

        if (const auto* boolean = dynamic_cast<const BoolExpr*>(&expression)) {
            return ExpressionFacts{
                TypeHint::Bool,
                true,
                boolean->value ? 1.0 : 0.0,
            };
        }

        if (const auto* variable = dynamic_cast<const VariableExpr*>(&expression)) {
            if (variable->name == "endl") {
                return ExpressionFacts{TypeHint::String, false, 0.0};
            }

            std::string lookup_name = variable->name;
            const std::size_t dot = lookup_name.find('.');
            if (dot != std::string::npos) {
                lookup_name = lookup_name.substr(0, dot);
            }

            const auto found = symbols.find(lookup_name);
            if (found == symbols.end()) {
                AddError(statement_id, "Variable potencialmente no definida: '" + lookup_name + "'.");
                return ExpressionFacts{TypeHint::Unknown, false, 0.0};
            }

            if (found->second.declaration_type == DeclarationType::Long ||
                found->second.declaration_type == DeclarationType::Byte) {
                return ExpressionFacts{TypeHint::Number, false, 0.0};
            }

            return ExpressionFacts{found->second.hint, false, 0.0};
        }

        if (const auto* list = dynamic_cast<const ListExpr*>(&expression)) {
            for (const auto& element : list->elements) {
                (void)InferExpression(*element, statement_id, symbols);
            }
            return ExpressionFacts{TypeHint::List, false, 0.0};
        }

        if (const auto* object = dynamic_cast<const ObjectExpr*>(&expression)) {
            for (const auto& entry : object->entries) {
                (void)InferExpression(*entry.value, statement_id, symbols);
            }
            return ExpressionFacts{TypeHint::Object, false, 0.0};
        }

        if (const auto* index = dynamic_cast<const IndexExpr*>(&expression)) {
            (void)InferExpression(*index->collection, statement_id, symbols);
            (void)InferExpression(*index->index, statement_id, symbols);
            return ExpressionFacts{TypeHint::Unknown, false, 0.0};
        }

        if (const auto* unary = dynamic_cast<const UnaryExpr*>(&expression)) {
            const ExpressionFacts operand = InferExpression(*unary->operand, statement_id, symbols);
            if (unary->op == UnaryOp::LogicalNot) {
                return ExpressionFacts{TypeHint::Bool, false, 0.0};
            }

            ExpressionFacts facts;
            facts.hint = TypeHint::Number;
            if (operand.is_constant_numeric) {
                facts.is_constant_numeric = true;
                facts.constant_numeric =
                    unary->op == UnaryOp::Negate ? -operand.constant_numeric : operand.constant_numeric;
            }
            return facts;
        }

        if (const auto* binary = dynamic_cast<const BinaryExpr*>(&expression)) {
            const ExpressionFacts lhs = InferExpression(*binary->lhs, statement_id, symbols);
            const ExpressionFacts rhs = InferExpression(*binary->rhs, statement_id, symbols);

            switch (binary->op) {
            case BinaryOp::Add:
                if (lhs.hint == TypeHint::String || rhs.hint == TypeHint::String) {
                    return ExpressionFacts{TypeHint::String, false, 0.0};
                }
                break;
            case BinaryOp::Equal:
            case BinaryOp::NotEqual:
            case BinaryOp::Less:
            case BinaryOp::LessEqual:
            case BinaryOp::Greater:
            case BinaryOp::GreaterEqual:
            case BinaryOp::LogicalAnd:
            case BinaryOp::LogicalOr:
                return ExpressionFacts{TypeHint::Bool, false, 0.0};
            case BinaryOp::Subtract:
            case BinaryOp::Multiply:
            case BinaryOp::Divide:
            case BinaryOp::Modulo:
            case BinaryOp::Power:
                break;
            }

            ExpressionFacts facts;
            facts.hint = TypeHint::Number;
            if (lhs.is_constant_numeric && rhs.is_constant_numeric) {
                facts.is_constant_numeric = true;
                switch (binary->op) {
                case BinaryOp::Add:
                    facts.constant_numeric = lhs.constant_numeric + rhs.constant_numeric;
                    break;
                case BinaryOp::Subtract:
                    facts.constant_numeric = lhs.constant_numeric - rhs.constant_numeric;
                    break;
                case BinaryOp::Multiply:
                    facts.constant_numeric = lhs.constant_numeric * rhs.constant_numeric;
                    break;
                case BinaryOp::Divide:
                    facts.constant_numeric = lhs.constant_numeric / rhs.constant_numeric;
                    break;
                case BinaryOp::Modulo:
                    facts.constant_numeric = std::fmod(lhs.constant_numeric, rhs.constant_numeric);
                    break;
                case BinaryOp::Power:
                    facts.constant_numeric = std::pow(lhs.constant_numeric, rhs.constant_numeric);
                    break;
                case BinaryOp::Equal:
                case BinaryOp::NotEqual:
                case BinaryOp::Less:
                case BinaryOp::LessEqual:
                case BinaryOp::Greater:
                case BinaryOp::GreaterEqual:
                case BinaryOp::LogicalAnd:
                case BinaryOp::LogicalOr:
                    facts.is_constant_numeric = false;
                    break;
                }
            }
            return facts;
        }

        if (const auto* call = dynamic_cast<const CallExpr*>(&expression)) {
            return AnalyzeCall(*call, statement_id, symbols);
        }

        return ExpressionFacts{TypeHint::Unknown, false, 0.0};
    }

    ExpressionFacts AnalyzeCall(
        const CallExpr& call,
        std::size_t statement_id,
        const SymbolTable& symbols) {
        for (const auto& argument : call.arguments) {
            if (argument.value != nullptr) {
                (void)InferExpression(*argument.value, statement_id, symbols);
            }
        }

        if (call.callee == "sum") {
            if (!math_imported_) {
                AddWarning(statement_id, "sum(a, b) requiere import math para evitar fallo en runtime.");
            }
            if (call.arguments.size() != 2) {
                AddError(statement_id, "sum(a, b) requiere 2 argumentos.");
            }
            return ExpressionFacts{TypeHint::Number, false, 0.0};
        }

        if (call.callee == "input") {
            if (call.arguments.size() > 1) {
                AddError(statement_id, "input() acepta 0 o 1 argumento.");
            }
            return ExpressionFacts{TypeHint::String, false, 0.0};
        }

        if (call.callee == "println") {
            if (call.arguments.size() > 1) {
                AddError(statement_id, "println() acepta 0 o 1 argumento.");
            }
            return ExpressionFacts{TypeHint::Unknown, false, 0.0};
        }

        if (call.callee == "printf") {
            if (call.arguments.empty()) {
                AddError(statement_id, "printf(format, ...args) requiere al menos 1 argumento.");
            }
            return ExpressionFacts{TypeHint::Number, false, 0.0};
        }

        if (call.callee == "read_file") {
            if (call.arguments.size() != 1) {
                AddError(statement_id, "read_file(path) requiere 1 argumento.");
            }
            return ExpressionFacts{TypeHint::String, false, 0.0};
        }

        if (call.callee == "write_file" || call.callee == "append_file") {
            if (call.arguments.size() != 2) {
                AddError(statement_id, call.callee + "(path, content) requiere 2 argumentos.");
            }
            return ExpressionFacts{TypeHint::Bool, false, 0.0};
        }

        if (call.callee == "file_exists") {
            if (call.arguments.size() != 1) {
                AddError(statement_id, "file_exists(path) requiere 1 argumento.");
            }
            return ExpressionFacts{TypeHint::Bool, false, 0.0};
        }

        if (call.callee == "now_ms") {
            if (!call.arguments.empty()) {
                AddError(statement_id, "now_ms() no acepta argumentos.");
            }
            return ExpressionFacts{TypeHint::Number, false, 0.0};
        }

        if (call.callee == "sleep_ms") {
            if (call.arguments.size() != 1) {
                AddError(statement_id, "sleep_ms(ms) requiere 1 argumento.");
            }
            return ExpressionFacts{TypeHint::Number, false, 0.0};
        }

        if (call.callee == "async_read_file") {
            if (call.arguments.size() != 1) {
                AddError(statement_id, "async_read_file(path) requiere 1 argumento.");
            }
            return ExpressionFacts{TypeHint::Number, false, 0.0};
        }

        if (call.callee == "task_ready") {
            if (call.arguments.size() != 1) {
                AddError(statement_id, "task_ready(task_id) requiere 1 argumento.");
            }
            return ExpressionFacts{TypeHint::Bool, false, 0.0};
        }

        if (call.callee == "await") {
            if (call.arguments.size() != 1) {
                AddError(statement_id, "await(task_id) requiere 1 argumento.");
            }
            return ExpressionFacts{TypeHint::Unknown, false, 0.0};
        }

        const auto function_it = functions_.find(call.callee);
        if (function_it == functions_.end()) {
            AddError(statement_id, "Llamada a funcion no definida: '" + call.callee + "'.");
            return ExpressionFacts{TypeHint::Unknown, false, 0.0};
        }

        const std::vector<bool>& by_ref = function_it->second.by_reference_params;
        if (call.arguments.size() != by_ref.size()) {
            AddError(statement_id, "Cantidad de argumentos incorrecta en '" + call.callee + "'.");
            return ExpressionFacts{TypeHint::Unknown, false, 0.0};
        }

        for (std::size_t i = 0; i < by_ref.size(); ++i) {
            if (!by_ref[i]) {
                continue;
            }

            if (call.arguments[i].value == nullptr) {
                AddError(statement_id, "Argumento vacio en llamada a '" + call.callee + "'.");
                continue;
            }

            const auto* variable = dynamic_cast<const VariableExpr*>(call.arguments[i].value.get());
            if (variable == nullptr) {
                AddError(statement_id, "Parametro por referencia requiere variable en '" + call.callee + "'.");
                continue;
            }

            std::string lookup_name = variable->name;
            const std::size_t dot = lookup_name.find('.');
            if (dot != std::string::npos) {
                lookup_name = lookup_name.substr(0, dot);
            }
            if (symbols.find(lookup_name) == symbols.end()) {
                AddError(statement_id, "Referencia a variable no definida: '" + lookup_name + "'.");
            }
        }

        return ExpressionFacts{TypeHint::Unknown, false, 0.0};
    }

    void ValidateTypedRangeIfNeeded(
        DeclarationType declaration_type,
        const ExpressionFacts& rhs,
        std::size_t statement_id,
        const std::string& variable_name) {
        if (declaration_type != DeclarationType::Long && declaration_type != DeclarationType::Byte) {
            return;
        }

        if (rhs.hint == TypeHint::String ||
            rhs.hint == TypeHint::Bool ||
            rhs.hint == TypeHint::List ||
            rhs.hint == TypeHint::Object) {
            AddError(statement_id, "Asignacion potencialmente invalida para variable tipada: '" + variable_name + "'.");
            return;
        }

        if (!rhs.is_constant_numeric) {
            return;
        }

        if (declaration_type == DeclarationType::Long) {
            constexpr double kLongMin = static_cast<double>(std::numeric_limits<long long>::min());
            constexpr double kLongUpperExclusive = 9223372036854775808.0;  // 2^63
            if (!std::isfinite(rhs.constant_numeric) ||
                rhs.constant_numeric < kLongMin ||
                rhs.constant_numeric >= kLongUpperExclusive) {
                AddError(statement_id, "Constante fuera de rango para long en '" + variable_name + "'.");
            }
            return;
        }

        if (!std::isfinite(rhs.constant_numeric) || rhs.constant_numeric < 0.0 || rhs.constant_numeric > 255.0) {
            AddError(statement_id, "Constante fuera de rango para byte en '" + variable_name + "'.");
        }
    }

    void AddError(std::size_t statement_id, const std::string& message) {
        if (report_ == nullptr) {
            return;
        }
        report_->errors.push_back(AnalysisDiagnostic{
            0,
            0,
            "Analisis estatico (sentencia " + std::to_string(statement_id) + "): " + message,
        });
    }

    void AddWarning(std::size_t statement_id, const std::string& message) {
        if (report_ == nullptr) {
            return;
        }
        report_->warnings.push_back(AnalysisDiagnostic{
            0,
            0,
            "Analisis estatico (sentencia " + std::to_string(statement_id) + "): " + message,
        });
    }

    AnalysisReport* report_ = nullptr;
    FunctionTable functions_;
    bool math_imported_ = false;
    std::size_t next_statement_id_ = 1;
};

}  // namespace

void StaticAnalyzer::Analyze(const Program& program, AnalysisReport* out_report) const {
    if (out_report == nullptr) {
        return;
    }

    out_report->errors.clear();
    out_report->warnings.clear();

    AnalyzerEngine engine(out_report);
    engine.Analyze(program);
}

}  // namespace clot::frontend
