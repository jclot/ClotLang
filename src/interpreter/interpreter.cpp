#include "clot/interpreter/interpreter.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>

#include "clot/runtime/i18n.hpp"

namespace clot::interpreter {

namespace {

using BigInt = runtime::Value::BigInt;

bool ReadNumeric(const runtime::Value& value, double* out_number, std::string* out_error) {
    bool ok = false;
    const double numeric = value.AsNumber(&ok);
    if (!ok) {
        if (out_error != nullptr) {
            *out_error = "La expresion requiere un valor numerico.";
        }
        return false;
    }

    *out_number = numeric;
    return true;
}

bool TryBigIntToSizeT(const BigInt& value, std::size_t* out_index) {
    if (out_index == nullptr || value < 0) {
        return false;
    }

    static const BigInt kMaxSizeT = [] {
        BigInt parsed;
        (void)BigInt::TryParse(std::to_string(std::numeric_limits<std::size_t>::max()), &parsed);
        return parsed;
    }();
    if (value > kMaxSizeT) {
        return false;
    }

    *out_index = value.convert_to<std::size_t>();
    return true;
}

bool PowBigInt(const BigInt& base, const BigInt& exponent, BigInt* out_result, std::string* out_error) {
    if (out_result == nullptr) {
        if (out_error != nullptr) {
            *out_error = "Error interno: salida nula en potencia entera.";
        }
        return false;
    }

    if (exponent < 0) {
        if (out_error != nullptr) {
            *out_error = "pow() no acepta exponente entero negativo para resultado entero.";
        }
        return false;
    }

    long long exponent_i64 = 0;
    if (!runtime::Value::TryBigIntToInt64(exponent, &exponent_i64)) {
        if (out_error != nullptr) {
            *out_error = "pow() exponente entero demasiado grande para computar.";
        }
        return false;
    }
    if (exponent_i64 < 0) {
        if (out_error != nullptr) {
            *out_error = "pow() no acepta exponente entero negativo para resultado entero.";
        }
        return false;
    }
    if (exponent_i64 > 1000000LL) {
        if (out_error != nullptr) {
            *out_error = "pow() exponente entero demasiado grande para computar.";
        }
        return false;
    }

    BigInt power = base;
    BigInt result = 1;
    unsigned long long exp = static_cast<unsigned long long>(exponent_i64);
    while (exp > 0) {
        if ((exp & 1ULL) != 0ULL) {
            result *= power;
        }
        exp >>= 1ULL;
        if (exp != 0ULL) {
            power *= power;
        }
    }

    *out_result = std::move(result);
    return true;
}

bool ReadListIndex(const runtime::Value& value, std::size_t* out_index, std::string* out_error) {
    BigInt integer_index;
    if (!value.AsBigInt(&integer_index)) {
        if (out_error != nullptr) {
            *out_error = "El indice de lista debe ser un entero finito.";
        }
        return false;
    }

    if (!TryBigIntToSizeT(integer_index, out_index)) {
        if (out_error != nullptr) {
            *out_error = "El indice de lista debe ser un entero finito.";
        }
        return false;
    }
    return true;
}

const char* TypeHintName(frontend::TypeHint hint) {
    switch (hint) {
    case frontend::TypeHint::Inferred:
        return "dynamic";
    case frontend::TypeHint::Int:
        return "int";
    case frontend::TypeHint::Double:
        return "double";
    case frontend::TypeHint::Float:
        return "float";
    case frontend::TypeHint::Decimal:
        return "decimal";
    case frontend::TypeHint::Long:
        return "long";
    case frontend::TypeHint::Byte:
        return "byte";
    case frontend::TypeHint::Char:
        return "char";
    case frontend::TypeHint::Tuple:
        return "tuple";
    case frontend::TypeHint::Set:
        return "set";
    case frontend::TypeHint::Map:
        return "map";
    case frontend::TypeHint::Function:
        return "function";
    case frontend::TypeHint::String:
        return "string";
    case frontend::TypeHint::Bool:
        return "bool";
    case frontend::TypeHint::List:
        return "list";
    case frontend::TypeHint::Object:
        return "object";
    case frontend::TypeHint::Null:
        return "null";
    }
    return "dynamic";
}

std::string TypeAnnotationName(const frontend::TypeAnnotation& annotation) {
    std::string name = TypeHintName(annotation.base);
    if (!annotation.type_args.empty()) {
        name.push_back('<');
        for (std::size_t i = 0; i < annotation.type_args.size(); ++i) {
            if (i > 0) {
                name += ", ";
            }
            name += TypeAnnotationName(annotation.type_args[i]);
        }
        name.push_back('>');
    }
    return name;
}

frontend::TypeAnnotation EffectiveTypeAnnotation(const frontend::TypeAnnotation& annotation,
                                                 frontend::TypeHint fallback_hint) {
    if (annotation.base != frontend::TypeHint::Inferred || !annotation.type_args.empty()) {
        return annotation;
    }
    frontend::TypeAnnotation effective;
    effective.base = fallback_hint;
    return effective;
}

bool TypeAnnotationEquals(const frontend::TypeAnnotation& lhs, const frontend::TypeAnnotation& rhs) {
    if (lhs.base != rhs.base || lhs.type_args.size() != rhs.type_args.size()) {
        return false;
    }
    for (std::size_t i = 0; i < lhs.type_args.size(); ++i) {
        if (!TypeAnnotationEquals(lhs.type_args[i], rhs.type_args[i])) {
            return false;
        }
    }
    return true;
}

frontend::TypeHint DeclarationTypeToTypeHint(frontend::DeclarationType declaration_type) {
    switch (declaration_type) {
    case frontend::DeclarationType::Int:
        return frontend::TypeHint::Int;
    case frontend::DeclarationType::Double:
        return frontend::TypeHint::Double;
    case frontend::DeclarationType::Float:
        return frontend::TypeHint::Float;
    case frontend::DeclarationType::Decimal:
        return frontend::TypeHint::Decimal;
    case frontend::DeclarationType::Long:
        return frontend::TypeHint::Long;
    case frontend::DeclarationType::Byte:
        return frontend::TypeHint::Byte;
    case frontend::DeclarationType::Char:
        return frontend::TypeHint::Char;
    case frontend::DeclarationType::List:
        return frontend::TypeHint::List;
    case frontend::DeclarationType::Tuple:
        return frontend::TypeHint::Tuple;
    case frontend::DeclarationType::Set:
        return frontend::TypeHint::Set;
    case frontend::DeclarationType::Map:
        return frontend::TypeHint::Map;
    case frontend::DeclarationType::Object:
        return frontend::TypeHint::Object;
    case frontend::DeclarationType::Function:
        return frontend::TypeHint::Function;
    case frontend::DeclarationType::Inferred:
    default:
        return frontend::TypeHint::Inferred;
    }
}

} // namespace

void Interpreter::SetEntryFilePath(const std::string& file_path) {
    entry_file_path_ = std::filesystem::path(file_path);
}

bool Interpreter::Execute(const frontend::Program& program, std::string* out_error) {
    environment_.clear();
    functions_.clear();
    interfaces_.clear();
    classes_.clear();
    imported_modules_.clear();
    loaded_module_programs_.clear();
    return_stack_.clear();
    class_execution_stack_.clear();
    constructor_execution_stack_.clear();
    constructor_super_called_stack_.clear();
    constructor_instance_stack_.clear();
    module_base_dirs_.clear();
    importing_modules_.clear();
    async_tasks_.clear();
    module_exports_cache_.clear();
    class_aliases_.clear();
    pending_exception_.reset();
    loop_depth_ = 0;
    switch_depth_ = 0;
    break_signal_ = false;
    continue_signal_ = false;
    defer_stack_.clear();
    next_async_task_id_ = 1;
    value_identity_cache_.clear();
    next_value_identity_id_ = 1;

    if (!entry_file_path_.empty()) {
        module_base_dirs_.push_back(entry_file_path_.parent_path());
    }

    defer_stack_.push_back({});
    bool top_level_ok = true;
    for (const auto& statement : program.statements) {
        if (!ExecuteStatement(*statement, out_error)) {
            top_level_ok = false;
            break;
        }
        if (break_signal_ || continue_signal_) {
            if (out_error != nullptr) {
                *out_error = "break/continue fuera de contexto de bucle.";
            }
            top_level_ok = false;
            break;
        }
    }

    if (!ExecuteDeferredStatementsForCurrentBlock(out_error)) {
        top_level_ok = false;
    }
    if (!defer_stack_.empty()) {
        defer_stack_.pop_back();
    }

    if (!top_level_ok) {
        if (pending_exception_.has_value()) {
            RuntimeExceptionRecord uncaught = *pending_exception_;
            pending_exception_.reset();

            if (out_error != nullptr) {
                std::string type_name = uncaught.type_name.empty() ? "RuntimeError" : uncaught.type_name;
                std::string message = uncaught.message;
                if (message.empty() && !out_error->empty()) {
                    message = *out_error;
                }
                if (message.empty()) {
                    message = "Exception lanzada.";
                }

                *out_error = "Excepcion no capturada: " + type_name + ": " + message;
            }
        }
        return false;
    }

    if (!return_stack_.empty()) {
        *out_error = "Error interno: pila de retorno inconsistente.";
        return false;
    }

    return true;
}

bool Interpreter::ExecuteBlock(const std::vector<std::unique_ptr<frontend::Statement>>& statements,
                               std::string* out_error) {
    defer_stack_.push_back({});
    bool ok = true;

    for (const auto& statement : statements) {
        if (!ExecuteStatement(*statement, out_error)) {
            ok = false;
            break;
        }

        if (!return_stack_.empty() && return_stack_.back().has_value()) {
            break;
        }

        if (break_signal_ || continue_signal_) {
            break;
        }
    }

    if (!ExecuteDeferredStatementsForCurrentBlock(out_error)) {
        ok = false;
    }
    if (!defer_stack_.empty()) {
        defer_stack_.pop_back();
    }
    return ok;
}

bool Interpreter::ExecuteDeferredStatementsForCurrentBlock(std::string* out_error) {
    if (defer_stack_.empty()) {
        return true;
    }

    std::vector<const frontend::Statement*>& deferred = defer_stack_.back();
    for (auto it = deferred.rbegin(); it != deferred.rend(); ++it) {
        if (*it == nullptr) {
            continue;
        }

        if (!ExecuteStatement(**it, out_error)) {
            return false;
        }
    }

    deferred.clear();
    return true;
}

bool Interpreter::ExecuteStatement(const frontend::Statement& statement, std::string* out_error) {
    if (const auto* assignment = dynamic_cast<const frontend::AssignmentStmt*>(&statement)) {
        runtime::Value value;
        bool has_value = false;

        if (assignment->declaration_type == frontend::DeclarationType::Function &&
            assignment->op == frontend::AssignmentOp::Set) {
            const auto* call_expr = dynamic_cast<const frontend::CallExpr*>(assignment->expr.get());
            if (call_expr != nullptr && call_expr->arguments.empty()) {
                const auto function_decl = functions_.find(call_expr->callee);
                if (function_decl != functions_.end()) {
                    value = runtime::Value(runtime::Value::FunctionRef{call_expr->callee});
                    has_value = true;
                } else {
                    const auto function_var = environment_.find(call_expr->callee);
                    if (function_var != environment_.end()) {
                        const auto* function_ref = function_var->second.value.AsFunctionRefValue();
                        if (function_ref != nullptr) {
                            value = runtime::Value(*function_ref);
                            has_value = true;
                        }
                    }
                }
            }
        }

        if (!has_value) {
            if (!EvaluateExpression(*assignment->expr, &value, out_error)) {
                return false;
            }
        }

        if (assignment->name.find('.') != std::string::npos) {
            frontend::VariableExpr target(assignment->name);
            return ApplyTargetMutation(target, assignment->op, value, out_error);
        }

        if (assignment->op != frontend::AssignmentOp::Set) {
            return ApplyVariableMutation(assignment->name, assignment->op, value, out_error);
        }

        return AssignValue(*assignment, value, out_error);
    }

    if (const auto* mutation = dynamic_cast<const frontend::MutationStmt*>(&statement)) {
        return ExecuteMutation(*mutation, out_error);
    }

    if (const auto* print = dynamic_cast<const frontend::PrintStmt*>(&statement)) {
        if (print->expr != nullptr) {
            runtime::Value value;
            if (!EvaluateExpression(*print->expr, &value, out_error)) {
                return false;
            }
            std::cout << value.ToString();
        }

        if (print->append_newline) {
            std::cout << std::endl;
        } else {
            std::cout << std::flush;
        }
        return true;
    }

    if (const auto* while_stmt = dynamic_cast<const frontend::WhileStmt*>(&statement)) {
        ++loop_depth_;
        while (true) {
            runtime::Value condition;
            if (!EvaluateExpression(*while_stmt->condition, &condition, out_error)) {
                --loop_depth_;
                return false;
            }
            if (!condition.AsBool()) {
                break;
            }

            if (!ExecuteBlock(while_stmt->body, out_error)) {
                --loop_depth_;
                return false;
            }

            if (break_signal_) {
                break_signal_ = false;
                break;
            }

            if (continue_signal_) {
                continue_signal_ = false;
                continue;
            }

            if (!return_stack_.empty() && return_stack_.back().has_value())
                break;
        }
        --loop_depth_;
        return true;
    }

    if (const auto* for_stmt = dynamic_cast<const frontend::ForStmt*>(&statement)) {
        return ExecuteFor(*for_stmt, out_error);
    }

    if (const auto* foreach_stmt = dynamic_cast<const frontend::ForEachStmt*>(&statement)) {
        return ExecuteForEach(*foreach_stmt, out_error);
    }

    if (const auto* do_while_stmt = dynamic_cast<const frontend::DoWhileStmt*>(&statement)) {
        return ExecuteDoWhile(*do_while_stmt, out_error);
    }

    if (const auto* switch_stmt = dynamic_cast<const frontend::SwitchStmt*>(&statement)) {
        return ExecuteSwitch(*switch_stmt, out_error);
    }

    if (const auto* conditional = dynamic_cast<const frontend::IfStmt*>(&statement)) {
        runtime::Value condition;
        if (!EvaluateExpression(*conditional->condition, &condition, out_error)) {
            return false;
        }

        if (condition.AsBool()) {
            return ExecuteBlock(conditional->then_branch, out_error);
        }

        return ExecuteBlock(conditional->else_branch, out_error);
    }

    if (const auto* declaration = dynamic_cast<const frontend::FunctionDeclStmt*>(&statement)) {
        functions_[declaration->name] = declaration;
        return true;
    }

    if (const auto* interface_decl = dynamic_cast<const frontend::InterfaceDeclStmt*>(&statement)) {
        return ExecuteInterfaceDeclaration(*interface_decl, out_error);
    }

    if (const auto* class_decl = dynamic_cast<const frontend::ClassDeclStmt*>(&statement)) {
        return ExecuteClassDeclaration(*class_decl, out_error);
    }

    if (const auto* import_stmt = dynamic_cast<const frontend::ImportStmt*>(&statement)) {
        std::string module_id;
        if (!ImportModule(import_stmt->module_name, &module_id, out_error)) {
            return false;
        }

        if (import_stmt->style == frontend::ImportStmt::Style::Module) {
            return true;
        }

        if (module_id == "math") {
            *out_error = "Import directo/alias para 'math' no soportado; use 'import math;'.";
            return false;
        }

        const auto exports_it = module_exports_cache_.find(module_id);
        if (exports_it == module_exports_cache_.end()) {
            *out_error = "Error interno: cache de exportaciones no encontrada para modulo '" + module_id + "'.";
            return false;
        }

        return BindImportedSymbol(*import_stmt, exports_it->second, out_error);
    }

    if (const auto* enum_decl = dynamic_cast<const frontend::EnumDeclStmt*>(&statement)) {
        runtime::Value::Object values;
        values.reserve(enum_decl->members.size());
        runtime::Value::Map value_to_name;
        value_to_name.entries.reserve(enum_decl->members.size());
        runtime::Value::Map name_to_value;
        name_to_value.entries.reserve(enum_decl->members.size());

        std::set<std::string> used_names;
        long long next_value = 0;
        for (const auto& member : enum_decl->members) {
            if (used_names.count(member) > 0) {
                *out_error = "Miembro enum duplicado: " + member;
                return false;
            }
            used_names.insert(member);
            values.push_back({member, runtime::Value(next_value)});
            value_to_name.entries.push_back({runtime::Value(next_value), runtime::Value(member)});
            name_to_value.entries.push_back({runtime::Value(member), runtime::Value(next_value)});
            ++next_value;
        }

        values.push_back({"__value_to_name", runtime::Value(std::move(value_to_name))});
        values.push_back({"__name_to_value", runtime::Value(std::move(name_to_value))});

        environment_[enum_decl->name] =
            runtime::VariableSlot{runtime::Value(std::move(values)), runtime::VariableKind::Dynamic};
        return true;
    }

    if (const auto* expression_stmt = dynamic_cast<const frontend::ExpressionStmt*>(&statement)) {
        if (const auto* call = dynamic_cast<const frontend::CallExpr*>(expression_stmt->expr.get())) {
            runtime::Value ignored;
            return ExecuteCall(*call, false, &ignored, out_error);
        }

        runtime::Value ignored;
        return EvaluateExpression(*expression_stmt->expr, &ignored, out_error);
    }

    if (const auto* return_stmt = dynamic_cast<const frontend::ReturnStmt*>(&statement)) {
        return ExecuteReturn(*return_stmt, out_error);
    }

    if (dynamic_cast<const frontend::BreakStmt*>(&statement) != nullptr) {
        if (loop_depth_ <= 0 && switch_depth_ <= 0) {
            *out_error = "break solo se permite dentro de bucles o switch.";
            return false;
        }
        break_signal_ = true;
        return true;
    }

    if (dynamic_cast<const frontend::ContinueStmt*>(&statement) != nullptr) {
        if (loop_depth_ <= 0) {
            *out_error = "continue solo se permite dentro de bucles.";
            return false;
        }
        continue_signal_ = true;
        return true;
    }

    if (dynamic_cast<const frontend::PassStmt*>(&statement) != nullptr) {
        return true;
    }

    if (const auto* defer_stmt = dynamic_cast<const frontend::DeferStmt*>(&statement)) {
        if (defer_stmt->statement == nullptr) {
            *out_error = "defer invalido: sentencia vacia.";
            return false;
        }
        if (defer_stack_.empty()) {
            *out_error = "defer fuera de bloque.";
            return false;
        }
        defer_stack_.back().push_back(defer_stmt->statement.get());
        return true;
    }

    if (const auto* try_catch_stmt = dynamic_cast<const frontend::TryCatchStmt*>(&statement)) {
        return ExecuteTryCatch(*try_catch_stmt, out_error);
    }

    *out_error = "Tipo de sentencia no soportado por el interprete.";
    return false;
}

bool Interpreter::ExecuteMutation(const frontend::MutationStmt& statement, std::string* out_error) {
    runtime::Value value;
    if (!EvaluateExpression(*statement.expr, &value, out_error)) {
        return false;
    }

    if (const auto* variable = dynamic_cast<const frontend::VariableExpr*>(statement.target.get())) {
        if (variable->name.find('.') == std::string::npos) {
            return ApplyVariableMutation(variable->name, statement.op, value, out_error);
        }
    }

    return ApplyTargetMutation(*statement.target, statement.op, value, out_error);
}

const frontend::ClassDeclStmt* Interpreter::FindClass(const std::string& class_name) const {
    const auto found = classes_.find(class_name);
    if (found == classes_.end()) {
        return nullptr;
    }
    return found->second.declaration;
}

bool Interpreter::IsClassInstance(const runtime::Value& value, std::string* out_class_name) const {
    const auto* object = value.AsObject();
    if (object == nullptr) {
        return false;
    }

    for (const auto& entry : *object) {
        if (entry.first == "__class__" && entry.second.IsString()) {
            if (out_class_name != nullptr) {
                *out_class_name = entry.second.ToString();
            }
            return true;
        }
    }
    return false;
}

bool Interpreter::CanAccessMember(frontend::MemberVisibility visibility, const std::string& owner_class) const {
    if (visibility == frontend::MemberVisibility::Public) {
        return true;
    }
    return HasClassContextAccess(owner_class);
}

bool Interpreter::HasClassContextAccess(const std::string& owner_class) const {
    return !class_execution_stack_.empty() && class_execution_stack_.back() == owner_class;
}

bool Interpreter::ResolveClassField(const frontend::ClassDeclStmt& declaration,
                                    const std::string& field_name,
                                    const frontend::ClassFieldDecl** out_field,
                                    std::string* out_owner_class) const {
    for (const auto& field : declaration.fields) {
        if (field.name == field_name) {
            if (out_field != nullptr) {
                *out_field = &field;
            }
            if (out_owner_class != nullptr) {
                *out_owner_class = declaration.name;
            }
            return true;
        }
    }

    if (!declaration.base_class.empty()) {
        const frontend::ClassDeclStmt* base = FindClass(declaration.base_class);
        if (base == nullptr) {
            return false;
        }
        return ResolveClassField(*base, field_name, out_field, out_owner_class);
    }
    return false;
}

bool Interpreter::ResolveClassMethod(const frontend::ClassDeclStmt& declaration,
                                     const std::string& method_name,
                                     const frontend::ClassMethodDecl** out_method,
                                     std::string* out_owner_class) const {
    for (const auto& method : declaration.methods) {
        if (method.name == method_name) {
            if (out_method != nullptr) {
                *out_method = &method;
            }
            if (out_owner_class != nullptr) {
                *out_owner_class = declaration.name;
            }
            return true;
        }
    }

    if (!declaration.base_class.empty()) {
        const frontend::ClassDeclStmt* base = FindClass(declaration.base_class);
        if (base == nullptr) {
            return false;
        }
        return ResolveClassMethod(*base, method_name, out_method, out_owner_class);
    }
    return false;
}

bool Interpreter::ResolveClassAccessor(const frontend::ClassDeclStmt& declaration,
                                       const std::string& property_name,
                                       bool setter,
                                       const frontend::ClassAccessorDecl** out_accessor,
                                       std::string* out_owner_class) const {
    for (const auto& accessor : declaration.accessors) {
        if (accessor.name == property_name && accessor.is_setter == setter) {
            if (out_accessor != nullptr) {
                *out_accessor = &accessor;
            }
            if (out_owner_class != nullptr) {
                *out_owner_class = declaration.name;
            }
            return true;
        }
    }

    if (!declaration.base_class.empty()) {
        const frontend::ClassDeclStmt* base = FindClass(declaration.base_class);
        if (base == nullptr) {
            return false;
        }
        return ResolveClassAccessor(*base, property_name, setter, out_accessor, out_owner_class);
    }
    return false;
}

bool Interpreter::ValidateOverrideRules(const frontend::ClassDeclStmt& declaration, std::string* out_error) const {
    const frontend::ClassDeclStmt* base = nullptr;
    if (!declaration.base_class.empty()) {
        base = FindClass(declaration.base_class);
        if (base == nullptr) {
            *out_error = "Clase base no definida: " + declaration.base_class;
            return false;
        }
    }

    for (const auto& method : declaration.methods) {
        const frontend::ClassMethodDecl* base_method = nullptr;
        std::string base_owner;
        const bool has_base_method =
            base != nullptr && ResolveClassMethod(*base, method.name, &base_method, &base_owner);

        if (method.is_override && !has_base_method) {
            *out_error = "Metodo override sin base: " + declaration.name + "." + method.name;
            return false;
        }

        if (!method.is_override && has_base_method) {
            *out_error = "Metodo sobrescribe base y requiere 'override': " + declaration.name + "." + method.name;
            return false;
        }

        if (has_base_method) {
            if (method.params.size() != base_method->params.size()) {
                *out_error = "Override invalido por aridad en metodo: " + declaration.name + "." + method.name;
                return false;
            }
            if (method.is_static != base_method->is_static) {
                *out_error = "Override invalido por static/instancia en metodo: " + declaration.name + "." + method.name;
                return false;
            }
            const frontend::TypeAnnotation method_return_annotation =
                EffectiveTypeAnnotation(method.return_annotation, method.return_type);
            const frontend::TypeAnnotation base_return_annotation =
                EffectiveTypeAnnotation(base_method->return_annotation, base_method->return_type);
            if (!TypeAnnotationEquals(method_return_annotation, base_return_annotation)) {
                *out_error = "Override invalido por retorno en metodo: " + declaration.name + "." + method.name;
                return false;
            }
            for (std::size_t i = 0; i < method.params.size(); ++i) {
                const frontend::TypeAnnotation method_param_annotation =
                    EffectiveTypeAnnotation(method.params[i].type_annotation, method.params[i].type_hint);
                const frontend::TypeAnnotation base_param_annotation =
                    EffectiveTypeAnnotation(base_method->params[i].type_annotation, base_method->params[i].type_hint);
                if (method.params[i].by_reference != base_method->params[i].by_reference ||
                    !TypeAnnotationEquals(method_param_annotation, base_param_annotation)) {
                    *out_error = "Override invalido por firma en metodo: " + declaration.name + "." + method.name;
                    return false;
                }
            }
        }
    }

    return true;
}

bool Interpreter::ValidateInterfaceImplementation(const frontend::ClassDeclStmt& declaration,
                                                  std::string* out_error) const {
    for (const auto& interface_name : declaration.interfaces) {
        const auto interface_it = interfaces_.find(interface_name);
        if (interface_it == interfaces_.end()) {
            *out_error = "Interface no definida: " + interface_name;
            return false;
        }

        const frontend::InterfaceDeclStmt* interface_decl = interface_it->second;
        for (const auto& signature : interface_decl->methods) {
            const frontend::ClassMethodDecl* method = nullptr;
            std::string owner;
            if (!ResolveClassMethod(declaration, signature.name, &method, &owner)) {
                *out_error = "Clase '" + declaration.name + "' no implementa metodo '" + signature.name +
                             "' de interface '" + interface_name + "'.";
                return false;
            }

            if (method->is_static) {
                *out_error = "Metodo de interface no puede implementarse como static: " + declaration.name + "." +
                             signature.name;
                return false;
            }

            if (method->visibility != frontend::MemberVisibility::Public) {
                *out_error = "Metodo de interface debe ser public: " + declaration.name + "." + signature.name;
                return false;
            }

            if (method->params.size() != signature.params.size()) {
                *out_error = "Firma incompatible en metodo de interface: " + declaration.name + "." + signature.name;
                return false;
            }

            const frontend::TypeAnnotation method_return_annotation =
                EffectiveTypeAnnotation(method->return_annotation, method->return_type);
            const frontend::TypeAnnotation signature_return_annotation =
                EffectiveTypeAnnotation(signature.return_annotation, signature.return_type);
            if (!TypeAnnotationEquals(method_return_annotation, signature_return_annotation)) {
                *out_error = "Retorno incompatible en metodo de interface: " + declaration.name + "." + signature.name;
                return false;
            }

            for (std::size_t i = 0; i < method->params.size(); ++i) {
                if (method->params[i].by_reference != signature.params[i].by_reference) {
                    *out_error = "Firma incompatible en metodo de interface: " + declaration.name + "." + signature.name;
                    return false;
                }

                const frontend::TypeAnnotation method_param_annotation =
                    EffectiveTypeAnnotation(method->params[i].type_annotation, method->params[i].type_hint);
                const frontend::TypeAnnotation signature_param_annotation =
                    EffectiveTypeAnnotation(signature.params[i].type_annotation, signature.params[i].type_hint);
                if (!TypeAnnotationEquals(method_param_annotation, signature_param_annotation)) {
                    *out_error = "Firma incompatible en metodo de interface: " + declaration.name + "." + signature.name;
                    return false;
                }
            }
        }
    }

    return true;
}

bool Interpreter::ResolveClassStaticField(const std::string& class_name,
                                          const std::string& field_name,
                                          runtime::Value** out_value,
                                          bool create_missing,
                                          std::string* out_error) {
    if (out_value == nullptr) {
        if (out_error != nullptr) {
            *out_error = "Error interno: out_value nulo en ResolveClassStaticField.";
        }
        return false;
    }

    const frontend::ClassDeclStmt* class_decl = FindClass(class_name);
    if (class_decl == nullptr) {
        *out_error = "Clase no definida: " + class_name;
        return false;
    }

    const frontend::ClassFieldDecl* field = nullptr;
    std::string owner_class;
    if (!ResolveClassField(*class_decl, field_name, &field, &owner_class)) {
        *out_error = "Campo no definido: " + class_name + "." + field_name;
        return false;
    }

    if (!field->is_static) {
        *out_error = "Campo de instancia requiere objeto: " + class_name + "." + field_name;
        return false;
    }

    if (!CanAccessMember(field->visibility, owner_class)) {
        *out_error = "Campo no accesible por visibilidad: " + class_name + "." + field_name;
        return false;
    }

    auto owner_it = classes_.find(owner_class);
    if (owner_it == classes_.end()) {
        *out_error = "Clase no definida: " + owner_class;
        return false;
    }

    auto static_field = owner_it->second.static_fields.find(field_name);
    if (static_field == owner_it->second.static_fields.end()) {
        if (!create_missing) {
            *out_error = "Campo static no inicializado: " + owner_class + "." + field_name;
            return false;
        }
        owner_it->second.static_fields[field_name] =
            runtime::VariableSlot{runtime::Value(nullptr), runtime::VariableKind::Dynamic};
        static_field = owner_it->second.static_fields.find(field_name);
    }

    *out_value = &static_field->second.value;
    return true;
}

bool Interpreter::ExecuteInterfaceDeclaration(const frontend::InterfaceDeclStmt& declaration, std::string* out_error) {
    if (interfaces_.find(declaration.name) != interfaces_.end()) {
        *out_error = "Interface duplicada: " + declaration.name;
        return false;
    }
    interfaces_[declaration.name] = &declaration;
    return true;
}

bool Interpreter::ExecuteClassDeclaration(const frontend::ClassDeclStmt& declaration, std::string* out_error) {
    if (classes_.find(declaration.name) != classes_.end()) {
        *out_error = "Clase duplicada: " + declaration.name;
        return false;
    }

    if (!declaration.base_class.empty() && FindClass(declaration.base_class) == nullptr) {
        *out_error = "Clase base no definida: " + declaration.base_class;
        return false;
    }

    ClassRuntimeInfo info;
    info.declaration = &declaration;
    classes_[declaration.name] = std::move(info);

    if (!ValidateOverrideRules(declaration, out_error) || !ValidateInterfaceImplementation(declaration, out_error)) {
        classes_.erase(declaration.name);
        return false;
    }

    std::set<std::string> seen_fields;
    for (const auto& field : declaration.fields) {
        if (!seen_fields.insert(field.name).second) {
            *out_error = "Campo duplicado en clase '" + declaration.name + "': " + field.name;
            classes_.erase(declaration.name);
            return false;
        }

        if (!field.is_static) {
            continue;
        }

        runtime::Value value(nullptr);
        if (field.default_value != nullptr) {
            if (!EvaluateExpression(*field.default_value, &value, out_error)) {
                classes_.erase(declaration.name);
                return false;
            }
        }

        const frontend::TypeAnnotation field_annotation =
            EffectiveTypeAnnotation(field.type_annotation, field.type_hint);
        if ((field_annotation.base != frontend::TypeHint::Inferred || !field_annotation.type_args.empty()) &&
            !(field.default_value == nullptr && value.IsNull())) {
            runtime::Value normalized;
            std::string type_error;
            if (!NormalizeValueForTypeAnnotation(field_annotation, value, &normalized, &type_error)) {
                *out_error = "Campo static '" + declaration.name + "." + field.name +
                             "' no coincide con type hint '" + TypeAnnotationName(field_annotation) + "': " + type_error;
                classes_.erase(declaration.name);
                return false;
            }
            value = std::move(normalized);
        }

        classes_[declaration.name].static_fields[field.name] =
            runtime::VariableSlot{value, runtime::VariableKind::Dynamic};
        if (field.is_readonly) {
            classes_[declaration.name].readonly_static_fields.insert(field.name);
        }
    }

    return true;
}

bool Interpreter::InitializeInstanceFields(const frontend::ClassDeclStmt& declaration,
                                           runtime::Value* instance,
                                           std::string* out_error) {
    if (instance == nullptr || !instance->IsObject()) {
        *out_error = "Error interno: instancia invalida en inicializacion de campos.";
        return false;
    }

    if (!declaration.base_class.empty()) {
        const frontend::ClassDeclStmt* base = FindClass(declaration.base_class);
        if (base == nullptr) {
            *out_error = "Clase base no definida: " + declaration.base_class;
            return false;
        }
        if (!InitializeInstanceFields(*base, instance, out_error)) {
            return false;
        }
    }

    for (const auto& field : declaration.fields) {
        if (field.is_static) {
            continue;
        }

        runtime::Value value(nullptr);
        if (field.default_value != nullptr) {
            if (!EvaluateExpression(*field.default_value, &value, out_error)) {
                return false;
            }
        }

        const frontend::TypeAnnotation field_annotation =
            EffectiveTypeAnnotation(field.type_annotation, field.type_hint);
        if ((field_annotation.base != frontend::TypeHint::Inferred || !field_annotation.type_args.empty()) &&
            !(field.default_value == nullptr && value.IsNull())) {
            runtime::Value normalized;
            std::string type_error;
            if (!NormalizeValueForTypeAnnotation(field_annotation, value, &normalized, &type_error)) {
                *out_error = "Campo '" + declaration.name + "." + field.name + "' no coincide con type hint '" +
                             TypeAnnotationName(field_annotation) + "': " + type_error;
                return false;
            }
            value = std::move(normalized);
        }

        runtime::Value* slot = instance->EnsureObjectProperty(field.name);
        if (slot == nullptr) {
            *out_error = "No se pudo inicializar campo de instancia: " + field.name;
            return false;
        }
        *slot = std::move(value);
    }

    return true;
}

bool Interpreter::ExecuteClassConstructor(const frontend::ClassDeclStmt& declaration,
                                          runtime::Value* instance,
                                          const frontend::CallExpr& call,
                                          std::string* out_error) {
    if (declaration.constructor_body.empty()) {
        if (!declaration.constructor_params.empty() || !call.arguments.empty()) {
            *out_error = "Numero incorrecto de argumentos para constructor de clase '" + declaration.name + "'.";
            return false;
        }
        if (!declaration.base_class.empty()) {
            const frontend::ClassDeclStmt* base = FindClass(declaration.base_class);
            if (base == nullptr) {
                *out_error = "Clase base no definida: " + declaration.base_class;
                return false;
            }
            frontend::CallExpr base_call("super", {});
            if (!ExecuteClassConstructor(*base, instance, base_call, out_error)) {
                return false;
            }
        }
        return true;
    }

    if (declaration.constructor_is_private && !HasClassContextAccess(declaration.name)) {
        *out_error = "Constructor privado no accesible para clase '" + declaration.name + "'.";
        return false;
    }

    if (!declaration.base_class.empty()) {
        const auto* first_statement = declaration.constructor_body.empty() ? nullptr : declaration.constructor_body.front().get();
        const auto* expression_stmt = dynamic_cast<const frontend::ExpressionStmt*>(first_statement);
        const auto* first_call = expression_stmt != nullptr
                                     ? dynamic_cast<const frontend::CallExpr*>(expression_stmt->expr.get())
                                     : nullptr;
        if (first_call == nullptr || first_call->callee != "super") {
            *out_error = "El constructor de '" + declaration.name +
                         "' debe invocar super(...) como primera sentencia.";
            return false;
        }
    }

    bool constructor_called_super = false;
    runtime::Value ignored(nullptr);
    if (!ExecuteClassCallable(
        declaration.name,
        declaration.name + "::__constructor__",
        frontend::TypeHint::Inferred,
        frontend::TypeAnnotation{},
        declaration.constructor_params,
        declaration.constructor_body,
        call,
        false,
        &ignored,
        out_error,
        instance,
        true,
        &constructor_called_super)) {
        return false;
    }

    if (!declaration.base_class.empty() && !constructor_called_super) {
        *out_error = "El constructor de '" + declaration.name + "' debe invocar super(...).";
        return false;
    }

    return true;
}

bool Interpreter::InstantiateClass(const frontend::ClassDeclStmt& declaration,
                                   const frontend::CallExpr& call,
                                   runtime::Value* out_value,
                                   std::string* out_error) {
    runtime::Value::Object entries;
    entries.push_back({"__class__", runtime::Value(declaration.name)});
    runtime::Value instance(std::move(entries));

    if (!InitializeInstanceFields(declaration, &instance, out_error)) {
        return false;
    }

    if (!ExecuteClassConstructor(declaration, &instance, call, out_error)) {
        return false;
    }

    *out_value = std::move(instance);
    return true;
}

bool Interpreter::ExecuteClassSetter(runtime::Value* instance,
                                     const std::string& class_name,
                                     const std::string& property_name,
                                     const runtime::Value& value,
                                     std::string* out_error) {
    if (instance == nullptr || !instance->IsObject()) {
        *out_error = "Error interno: instancia invalida para setter de clase.";
        return false;
    }

    const frontend::ClassDeclStmt* class_decl = FindClass(class_name);
    if (class_decl == nullptr) {
        *out_error = "Clase no definida: " + class_name;
        return false;
    }

    const frontend::ClassAccessorDecl* accessor = nullptr;
    std::string owner_class;
    if (!ResolveClassAccessor(*class_decl, property_name, true, &accessor, &owner_class)) {
        return false;
    }

    if (!CanAccessMember(accessor->visibility, owner_class)) {
        *out_error = "Setter no accesible por visibilidad: " + class_name + "." + property_name;
        return false;
    }

    runtime::Value normalized_value = value;
    const frontend::TypeAnnotation setter_annotation =
        EffectiveTypeAnnotation(accessor->setter_param_annotation, accessor->setter_param_type);
    if (setter_annotation.base != frontend::TypeHint::Inferred || !setter_annotation.type_args.empty()) {
        std::string type_error;
        if (!NormalizeValueForTypeAnnotation(setter_annotation, value, &normalized_value, &type_error)) {
            *out_error = "Setter '" + class_name + "." + property_name +
                         "' recibio valor incompatible con type hint '" +
                         TypeAnnotationName(setter_annotation) + "': " + type_error;
            return false;
        }
    }

    auto caller_environment = environment_;
    auto local_environment = environment_;
    local_environment["this"] = runtime::VariableSlot{*instance, runtime::VariableKind::Dynamic};
    local_environment[accessor->setter_param_name] =
        runtime::VariableSlot{normalized_value, runtime::VariableKind::Dynamic};

    class_execution_stack_.push_back(owner_class);
    environment_ = std::move(local_environment);
    return_stack_.push_back(std::nullopt);

    const bool ok = ExecuteBlock(accessor->body, out_error);
    return_stack_.pop_back();

    if (ok) {
        const auto this_it = environment_.find("this");
        if (this_it != environment_.end()) {
            *instance = this_it->second.value;
        }
    }

    environment_ = std::move(caller_environment);
    if (!class_execution_stack_.empty()) {
        class_execution_stack_.pop_back();
    }
    return ok;
}

bool Interpreter::TryExecuteClassGetter(runtime::Value* instance,
                                        const std::string& class_name,
                                        const std::string& property_name,
                                        runtime::Value* out_value,
                                        std::string* out_error) {
    if (instance == nullptr || out_value == nullptr || !instance->IsObject()) {
        if (out_error != nullptr) {
            *out_error = "Error interno: instancia invalida para getter de clase.";
        }
        return false;
    }

    const frontend::ClassDeclStmt* class_decl = FindClass(class_name);
    if (class_decl == nullptr) {
        *out_error = "Clase no definida: " + class_name;
        return false;
    }

    const frontend::ClassAccessorDecl* accessor = nullptr;
    std::string owner_class;
    if (!ResolveClassAccessor(*class_decl, property_name, false, &accessor, &owner_class)) {
        return false;
    }

    if (!CanAccessMember(accessor->visibility, owner_class)) {
        *out_error = "Getter no accesible por visibilidad: " + class_name + "." + property_name;
        return false;
    }

    auto caller_environment = environment_;
    auto local_environment = environment_;
    local_environment["this"] = runtime::VariableSlot{*instance, runtime::VariableKind::Dynamic};

    class_execution_stack_.push_back(owner_class);
    environment_ = std::move(local_environment);
    return_stack_.push_back(std::nullopt);

    const bool ok = ExecuteBlock(accessor->body, out_error);
    std::optional<runtime::Value> returned = return_stack_.back();
    return_stack_.pop_back();

    if (ok) {
        const auto this_it = environment_.find("this");
        if (this_it != environment_.end()) {
            *instance = this_it->second.value;
        }
    }

    environment_ = std::move(caller_environment);
    if (!class_execution_stack_.empty()) {
        class_execution_stack_.pop_back();
    }

    if (!ok) {
        return false;
    }

    *out_value = returned.has_value() ? *returned : runtime::Value(nullptr);
    return true;
}

bool Interpreter::ExecuteReturn(const frontend::ReturnStmt& statement, std::string* out_error) {
    if (return_stack_.empty()) {
        *out_error = "return solo se permite dentro de una funcion.";
        return false;
    }

    runtime::Value value(nullptr);
    if (statement.expr != nullptr) {
        if (!EvaluateExpression(*statement.expr, &value, out_error)) {
            return false;
        }
    }

    return_stack_.back() = std::move(value);
    return true;
}

bool Interpreter::CollectForEachElements(const runtime::Value& collection,
                                         std::vector<runtime::Value>* out_elements,
                                         std::string* out_error) const {
    if (out_elements == nullptr) {
        if (out_error != nullptr) {
            *out_error = "Error interno: salida nula en for-each.";
        }
        return false;
    }
    out_elements->clear();

    if (const auto* list = collection.AsList()) {
        out_elements->insert(out_elements->end(), list->begin(), list->end());
        return true;
    }
    if (const auto* tuple = collection.AsTuple()) {
        out_elements->insert(out_elements->end(), tuple->begin(), tuple->end());
        return true;
    }
    if (const auto* set = collection.AsSet()) {
        out_elements->insert(out_elements->end(), set->begin(), set->end());
        return true;
    }
    if (const auto* map = collection.AsMap()) {
        for (const auto& entry : *map) {
            out_elements->push_back(entry.first);
        }
        return true;
    }
    if (const auto* object = collection.AsObject()) {
        for (const auto& entry : *object) {
            out_elements->push_back(runtime::Value(entry.first));
        }
        return true;
    }
    if (collection.IsString()) {
        const std::string text = collection.ToString();
        for (char ch : text) {
            out_elements->push_back(runtime::Value(ch));
        }
        return true;
    }

    if (out_error != nullptr) {
        *out_error = "for-each requiere list, tuple, set, map, object o string.";
    }
    return false;
}

bool Interpreter::ExecuteFor(const frontend::ForStmt& statement, std::string* out_error) {
    if (statement.initializer != nullptr) {
        if (!ExecuteStatement(*statement.initializer, out_error)) {
            return false;
        }
    }

    ++loop_depth_;
    while (true) {
        if (statement.condition != nullptr) {
            runtime::Value condition;
            if (!EvaluateExpression(*statement.condition, &condition, out_error)) {
                --loop_depth_;
                return false;
            }
            if (!condition.AsBool()) {
                break;
            }
        }

        if (!ExecuteBlock(statement.body, out_error)) {
            --loop_depth_;
            return false;
        }

        if (break_signal_) {
            break_signal_ = false;
            break;
        }

        if (continue_signal_) {
            continue_signal_ = false;
        }

        if (!return_stack_.empty() && return_stack_.back().has_value()) {
            break;
        }

        if (statement.update != nullptr) {
            if (!ExecuteStatement(*statement.update, out_error)) {
                --loop_depth_;
                return false;
            }
            if (break_signal_) {
                break_signal_ = false;
                break;
            }
            if (continue_signal_) {
                continue_signal_ = false;
            }
            if (!return_stack_.empty() && return_stack_.back().has_value()) {
                break;
            }
        }
    }
    --loop_depth_;
    return true;
}

bool Interpreter::ExecuteForEach(const frontend::ForEachStmt& statement, std::string* out_error) {
    runtime::Value collection;
    if (!EvaluateExpression(*statement.collection, &collection, out_error)) {
        return false;
    }

    std::vector<runtime::Value> elements;
    if (!CollectForEachElements(collection, &elements, out_error)) {
        return false;
    }

    std::optional<runtime::VariableSlot> previous_slot;
    const auto existing = environment_.find(statement.variable_name);
    if (existing != environment_.end()) {
        previous_slot = existing->second;
    }

    runtime::VariableKind kind = runtime::VariableKind::Dynamic;
    switch (statement.variable_type) {
    case frontend::DeclarationType::Int:
        kind = runtime::VariableKind::Int;
        break;
    case frontend::DeclarationType::Double:
        kind = runtime::VariableKind::Double;
        break;
    case frontend::DeclarationType::Float:
        kind = runtime::VariableKind::Float;
        break;
    case frontend::DeclarationType::Decimal:
        kind = runtime::VariableKind::Decimal;
        break;
    case frontend::DeclarationType::Long:
        kind = runtime::VariableKind::Long;
        break;
    case frontend::DeclarationType::Byte:
        kind = runtime::VariableKind::Byte;
        break;
    case frontend::DeclarationType::Char:
        kind = runtime::VariableKind::Char;
        break;
    case frontend::DeclarationType::List:
        kind = runtime::VariableKind::List;
        break;
    case frontend::DeclarationType::Tuple:
        kind = runtime::VariableKind::Tuple;
        break;
    case frontend::DeclarationType::Set:
        kind = runtime::VariableKind::Set;
        break;
    case frontend::DeclarationType::Map:
        kind = runtime::VariableKind::Map;
        break;
    case frontend::DeclarationType::Object:
        kind = runtime::VariableKind::Object;
        break;
    case frontend::DeclarationType::Function:
        kind = runtime::VariableKind::Function;
        break;
    case frontend::DeclarationType::Inferred:
    default:
        kind = runtime::VariableKind::Dynamic;
        break;
    }

    const frontend::TypeAnnotation variable_annotation =
        EffectiveTypeAnnotation(statement.variable_annotation, DeclarationTypeToTypeHint(statement.variable_type));
    const bool has_annotation =
        variable_annotation.base != frontend::TypeHint::Inferred || !variable_annotation.type_args.empty();

    ++loop_depth_;
    for (const auto& element : elements) {
        runtime::Value normalized = element;
        if (has_annotation) {
            if (!NormalizeValueForTypeAnnotation(variable_annotation, element, &normalized, out_error)) {
                --loop_depth_;
                if (previous_slot.has_value()) {
                    environment_[statement.variable_name] = *previous_slot;
                } else {
                    environment_.erase(statement.variable_name);
                }
                return false;
            }
        } else if (kind != runtime::VariableKind::Dynamic) {
            if (!NormalizeValueForKind(kind, element, &normalized, out_error)) {
                --loop_depth_;
                if (previous_slot.has_value()) {
                    environment_[statement.variable_name] = *previous_slot;
                } else {
                    environment_.erase(statement.variable_name);
                }
                return false;
            }
        }

        environment_[statement.variable_name] = runtime::VariableSlot{normalized, kind, statement.variable_is_const};

        if (!ExecuteBlock(statement.body, out_error)) {
            --loop_depth_;
            if (previous_slot.has_value()) {
                environment_[statement.variable_name] = *previous_slot;
            } else {
                environment_.erase(statement.variable_name);
            }
            return false;
        }

        if (break_signal_) {
            break_signal_ = false;
            break;
        }

        if (continue_signal_) {
            continue_signal_ = false;
            continue;
        }

        if (!return_stack_.empty() && return_stack_.back().has_value()) {
            break;
        }
    }
    --loop_depth_;

    if (previous_slot.has_value()) {
        environment_[statement.variable_name] = *previous_slot;
    } else {
        environment_.erase(statement.variable_name);
    }
    return true;
}

bool Interpreter::ExecuteDoWhile(const frontend::DoWhileStmt& statement, std::string* out_error) {
    ++loop_depth_;
    while (true) {
        if (!ExecuteBlock(statement.body, out_error)) {
            --loop_depth_;
            return false;
        }

        if (break_signal_) {
            break_signal_ = false;
            break;
        }

        if (continue_signal_) {
            continue_signal_ = false;
        }

        if (!return_stack_.empty() && return_stack_.back().has_value()) {
            break;
        }

        runtime::Value condition;
        if (!EvaluateExpression(*statement.condition, &condition, out_error)) {
            --loop_depth_;
            return false;
        }
        if (!condition.AsBool()) {
            break;
        }
    }
    --loop_depth_;
    return true;
}

bool Interpreter::ExecuteSwitch(const frontend::SwitchStmt& statement, std::string* out_error) {
    runtime::Value switch_value;
    if (!EvaluateExpression(*statement.value, &switch_value, out_error)) {
        return false;
    }

    std::size_t default_index = statement.cases.size();
    std::size_t matched_index = statement.cases.size();
    for (std::size_t i = 0; i < statement.cases.size(); ++i) {
        const auto& switch_case = statement.cases[i];
        if (switch_case.is_default) {
            if (default_index == statement.cases.size()) {
                default_index = i;
            }
            continue;
        }
        if (switch_case.match_expr == nullptr) {
            continue;
        }

        runtime::Value case_value;
        if (!EvaluateExpression(*switch_case.match_expr, &case_value, out_error)) {
            return false;
        }
        if (switch_value.Equals(case_value)) {
            matched_index = i;
            break;
        }
    }

    if (matched_index == statement.cases.size()) {
        matched_index = default_index;
    }
    if (matched_index == statement.cases.size()) {
        return true;
    }

    ++switch_depth_;
    for (std::size_t i = matched_index; i < statement.cases.size(); ++i) {
        if (!ExecuteBlock(statement.cases[i].body, out_error)) {
            --switch_depth_;
            return false;
        }

        if (break_signal_) {
            break_signal_ = false;
            break;
        }

        if (continue_signal_) {
            break;
        }

        if (!return_stack_.empty() && return_stack_.back().has_value()) {
            break;
        }
    }
    --switch_depth_;
    return true;
}

bool Interpreter::IsClassTypeOrDerived(const std::string& type_name, const std::string& base_type_name) const {
    if (type_name.empty() || base_type_name.empty()) {
        return false;
    }
    if (type_name == base_type_name) {
        return true;
    }

    const frontend::ClassDeclStmt* current = FindClass(type_name);
    std::set<std::string> visited;
    while (current != nullptr && !current->base_class.empty()) {
        if (!visited.insert(current->name).second) {
            break;
        }
        if (current->base_class == base_type_name) {
            return true;
        }
        current = FindClass(current->base_class);
    }
    return false;
}

std::string Interpreter::InferRuntimeExceptionTypeName(const std::string& runtime_error) const {
    std::string lowered = runtime_error;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (lowered.find("assert") != std::string::npos) {
        return "AssertionError";
    }
    if (lowered.find("import") != std::string::npos) {
        if ((lowered.find("error importando modulo") != std::string::npos ||
             lowered.find("error importing module") != std::string::npos) &&
            (lowered.find("no se pudo abrir el archivo") != std::string::npos ||
             lowered.find("could not open file") != std::string::npos)) {
            return "ModuleNotFoundError";
        }
        return "ImportError";
    }
    if (lowered.find("permiso denegado") != std::string::npos ||
        lowered.find("permission denied") != std::string::npos) {
        return "PermissionError";
    }
    if (lowered.find("archivo ya existe") != std::string::npos ||
        lowered.find("file already exists") != std::string::npos) {
        return "FileExistsError";
    }
    if (lowered.find("archivo cerrado") != std::string::npos ||
        lowered.find("file closed") != std::string::npos) {
        return "FileClosedError";
    }
    if (lowered.find("no se pudo abrir el archivo") != std::string::npos ||
        lowered.find("could not open file") != std::string::npos) {
        return "FileNotFoundError";
    }
    if (lowered.find("error leyendo el archivo") != std::string::npos ||
        lowered.find("error escribiendo el archivo") != std::string::npos ||
        lowered.find("error reading file") != std::string::npos ||
        lowered.find("error writing file") != std::string::npos) {
        return "IOError";
    }
    if (lowered.find("variable no definida") != std::string::npos ||
        lowered.find("funcion no definida") != std::string::npos ||
        lowered.find("undefined variable") != std::string::npos ||
        lowered.find("undefined function") != std::string::npos) {
        return "NameError";
    }
    if (lowered.find("propiedad no encontrada") != std::string::npos ||
        lowered.find("property not found") != std::string::npos ||
        lowered.find("acceso de propiedad invalido") != std::string::npos ||
        lowered.find("invalid property access") != std::string::npos ||
        lowered.find("no se puede acceder propiedad en un valor no objeto") != std::string::npos ||
        lowered.find("cannot access property on non-object value") != std::string::npos ||
        lowered.find("clave no encontrada") != std::string::npos ||
        lowered.find("key not found") != std::string::npos) {
        return "AttributeError";
    }
    if (lowered.find("indice fuera de rango") != std::string::npos ||
        lowered.find("index out of bounds") != std::string::npos ||
        lowered.find("tuple index out of bounds") != std::string::npos) {
        return "IndexError";
    }
    if (lowered.find("fuera de rango") != std::string::npos ||
        lowered.find("out of range") != std::string::npos ||
        lowered.find("demasiado grande") != std::string::npos ||
        lowered.find("too large") != std::string::npos ||
        lowered.find("negativo") != std::string::npos ||
        lowered.find("negative") != std::string::npos ||
        lowered.find(">= 0") != std::string::npos ||
        lowered.find("0-255") != std::string::npos) {
        return "RangeError";
    }
    if (lowered.find("acepta 0 o 1 argumento") != std::string::npos ||
        lowered.find("accepts 0 or 1 argument") != std::string::npos) {
        return "TooManyArgumentsError";
    }
    if ((lowered.find("requiere") != std::string::npos &&
         lowered.find("argumento") != std::string::npos) ||
        (lowered.find("requires") != std::string::npos &&
         lowered.find("argument") != std::string::npos)) {
        return "MissingArgumentError";
    }
    if (lowered.find("argumento") != std::string::npos ||
        lowered.find("arguments") != std::string::npos ||
        lowered.find("argument") != std::string::npos) {
        return "ArgumentError";
    }
    if (lowered.find("type hint") != std::string::npos ||
        lowered.find("requiere un") != std::string::npos ||
        lowered.find("requires a") != std::string::npos ||
        lowered.find("incompatible") != std::string::npos) {
        return "TypeError";
    }
    if (lowered.find("valor invalido para") != std::string::npos ||
        lowered.find("invalid value for") != std::string::npos ||
        lowered.find("debe ser") != std::string::npos ||
        lowered.find("must be") != std::string::npos) {
        return "ValueError";
    }
    return "RuntimeError";
}

Interpreter::RuntimeExceptionRecord Interpreter::BuildRuntimeExceptionFromError(const std::string& runtime_error) const {
    RuntimeExceptionRecord exception;
    exception.type_name = InferRuntimeExceptionTypeName(runtime_error);
    exception.payload = runtime::Value(runtime::TranslateDiagnostic(runtime_error));
    exception.message = runtime_error;
    return exception;
}

bool Interpreter::ExceptionMatchesCatchType(const RuntimeExceptionRecord& exception, const std::string& catch_type) const {
    if (catch_type.empty()) {
        return true;
    }
    if (exception.type_name == catch_type) {
        return true;
    }
    if (catch_type == "Exception") {
        return true;
    }
    if (IsClassTypeOrDerived(exception.type_name, catch_type)) {
        return true;
    }

    const auto builtin_base_type = [](const std::string& type_name) -> std::string {
        if (type_name == "MissingArgumentError" ||
            type_name == "TooManyArgumentsError") {
            return "ArgumentError";
        }
        if (type_name == "ArgumentError") {
            return "TypeError";
        }
        if (type_name == "TypeError") {
            return "RuntimeError";
        }
        if (type_name == "IndexError") {
            return "RangeError";
        }
        if (type_name == "RangeError") {
            return "ValueError";
        }
        if (type_name == "ValueError") {
            return "RuntimeError";
        }
        if (type_name == "NameError" ||
            type_name == "AttributeError" ||
            type_name == "AssertionError" ||
            type_name == "ImportError") {
            return "RuntimeError";
        }
        if (type_name == "FileNotFoundError" ||
            type_name == "PermissionError" ||
            type_name == "FileExistsError" ||
            type_name == "FileClosedError") {
            return "IOError";
        }
        if (type_name == "IOError") {
            return "RuntimeError";
        }
        if (type_name == "ModuleNotFoundError") {
            return "ImportError";
        }
        if (type_name == "RuntimeError") {
            return "Exception";
        }
        return "";
    };

    std::string current_type = exception.type_name;
    std::set<std::string> visited_builtin_types;
    while (!current_type.empty() && visited_builtin_types.insert(current_type).second) {
        if (current_type == catch_type) {
            return true;
        }
        current_type = builtin_base_type(current_type);
    }

    return false;
}

bool Interpreter::RaiseExceptionValue(const runtime::Value& value, std::string* out_error) {
    RuntimeExceptionRecord exception;
    exception.type_name = "RuntimeError";
    exception.payload = value;

    std::string class_name;
    if (IsClassInstance(value, &class_name)) {
        exception.type_name = class_name;
        if (const runtime::Value* message_value = value.GetObjectProperty("message")) {
            exception.message = message_value->ToString();
        }
    }

    if (exception.message.empty()) {
        exception.message = value.ToString();
    }
    if (exception.message.empty()) {
        exception.message = "Exception lanzada.";
    }

    pending_exception_ = exception;
    if (out_error != nullptr) {
        *out_error = exception.message;
    }
    return false;
}

bool Interpreter::ExecuteTryCatch(const frontend::TryCatchStmt& statement, std::string* out_error) {
    std::string try_error;
    bool try_ok = ExecuteBlock(statement.try_branch, &try_error);

    bool propagate_exception = false;
    RuntimeExceptionRecord pending;
    if (!try_ok) {
        pending = pending_exception_.has_value()
                      ? *pending_exception_
                      : BuildRuntimeExceptionFromError(try_error);
        pending_exception_.reset();
        if (pending.message.empty()) {
            pending.message = try_error;
        }

        bool handled_by_catch = false;
        if (statement.has_catch) {
            if (statement.catch_type.empty() || ExceptionMatchesCatchType(pending, statement.catch_type)) {
                handled_by_catch = true;

                std::optional<runtime::VariableSlot> previous_slot;
                if (!statement.error_binding.empty()) {
                    const auto existing = environment_.find(statement.error_binding);
                    if (existing != environment_.end()) {
                        previous_slot = existing->second;
                    }

                    environment_[statement.error_binding] = runtime::VariableSlot{
                        pending.payload,
                        runtime::VariableKind::Dynamic,
                        false,
                    };
                }

                std::string catch_error;
                const bool catch_ok = ExecuteBlock(statement.catch_branch, &catch_error);
                if (!statement.error_binding.empty()) {
                    if (previous_slot.has_value()) {
                        environment_[statement.error_binding] = *previous_slot;
                    } else {
                        environment_.erase(statement.error_binding);
                    }
                }

                if (!catch_ok) {
                    if (out_error != nullptr) {
                        *out_error = catch_error;
                    }
                    return false;
                }
            }
        }

        if (!handled_by_catch) {
            propagate_exception = true;
        }
    }

    if (!statement.finally_branch.empty()) {
        std::string finally_error;
        if (!ExecuteBlock(statement.finally_branch, &finally_error)) {
            if (out_error != nullptr) {
                *out_error = finally_error;
            }
            return false;
        }
    }

    if (propagate_exception) {
        pending_exception_ = pending;
        if (out_error != nullptr) {
            *out_error = pending.message;
        }
        return false;
    }

    pending_exception_.reset();
    return true;
}

bool Interpreter::EvaluateExpression(const frontend::Expr& expression, runtime::Value* out_value,
                                     std::string* out_error) {
    if (const auto* number = dynamic_cast<const frontend::NumberExpr*>(&expression)) {
        if (number->is_integer_literal) {
            runtime::Value::BigInt integer;
            if (!runtime::Value::TryParseBigInt(number->lexeme, &integer)) {
                *out_error = "Numero invalido: '" + number->lexeme + "'.";
                return false;
            }
            *out_value = runtime::Value(std::move(integer));
            return true;
        }
        *out_value = runtime::Value(number->value);
        return true;
    }

    if (const auto* text = dynamic_cast<const frontend::StringExpr*>(&expression)) {
        *out_value = runtime::Value(text->value);
        return true;
    }

    if (const auto* boolean = dynamic_cast<const frontend::BoolExpr*>(&expression)) {
        *out_value = runtime::Value(boolean->value);
        return true;
    }

    if (const auto* character = dynamic_cast<const frontend::CharExpr*>(&expression)) {
        *out_value = runtime::Value(character->value);
        return true;
    }

    if (dynamic_cast<const frontend::NullExpr*>(&expression) != nullptr) {
        *out_value = runtime::Value(nullptr);
        return true;
    }

    if (const auto* variable = dynamic_cast<const frontend::VariableExpr*>(&expression)) {
        return ResolveVariable(variable->name, out_value, out_error);
    }

    if (const auto* list = dynamic_cast<const frontend::ListExpr*>(&expression)) {
        runtime::Value::List values;
        values.reserve(list->elements.size());
        for (const auto& element : list->elements) {
            runtime::Value evaluated;
            if (!EvaluateExpression(*element, &evaluated, out_error)) {
                return false;
            }
            values.push_back(std::move(evaluated));
        }

        *out_value = runtime::Value(std::move(values));
        return true;
    }

    if (const auto* object = dynamic_cast<const frontend::ObjectExpr*>(&expression)) {
        runtime::Value::Object values;
        values.reserve(object->entries.size());
        for (const auto& entry : object->entries) {
            runtime::Value evaluated;
            if (!EvaluateExpression(*entry.value, &evaluated, out_error)) {
                return false;
            }
            values.push_back({entry.key, std::move(evaluated)});
        }

        *out_value = runtime::Value(std::move(values));
        return true;
    }

    if (const auto* index = dynamic_cast<const frontend::IndexExpr*>(&expression)) {
        runtime::Value collection;
        runtime::Value index_value;

        if (!EvaluateExpression(*index->collection, &collection, out_error)) {
            return false;
        }
        if (!EvaluateExpression(*index->index, &index_value, out_error)) {
            return false;
        }

        if (const runtime::Value::List* list = collection.AsList()) {
            std::size_t integer_index = 0;
            if (!ReadListIndex(index_value, &integer_index, out_error)) {
                return false;
            }

            if (integer_index >= list->size()) {
                *out_error = "Indice fuera de rango en lista.";
                return false;
            }

            *out_value = (*list)[integer_index];
            return true;
        }

        if (const std::vector<runtime::Value>* tuple = collection.AsTuple()) {
            std::size_t integer_index = 0;
            if (!ReadListIndex(index_value, &integer_index, out_error)) {
                return false;
            }

            if (integer_index >= tuple->size()) {
                *out_error = "Indice fuera de rango en tuple.";
                return false;
            }

            *out_value = (*tuple)[integer_index];
            return true;
        }

        if (const auto* map = collection.AsMap()) {
            for (const auto& entry : *map) {
                if (entry.first.Equals(index_value)) {
                    *out_value = entry.second;
                    return true;
                }
            }
            *out_error = "Clave no encontrada en map.";
            return false;
        }

        if (const auto* object = collection.AsObject()) {
            const std::string key = index_value.ToString();
            for (const auto& entry : *object) {
                if (entry.first == key) {
                    *out_value = entry.second;
                    return true;
                }
            }
            *out_error = "Propiedad no encontrada: " + key;
            return false;
        }

        *out_error = "Solo se puede indexar list, tuple, map u object con [].";
        return false;
    }

    if (const auto* call = dynamic_cast<const frontend::CallExpr*>(&expression)) {
        return ExecuteCall(*call, true, out_value, out_error);
    }

    if (const auto* unary = dynamic_cast<const frontend::UnaryExpr*>(&expression)) {
        runtime::Value operand;
        if (!EvaluateExpression(*unary->operand, &operand, out_error)) {
            return false;
        }

        return EvaluateUnary(unary->op, operand, out_value, out_error);
    }

    if (const auto* binary = dynamic_cast<const frontend::BinaryExpr*>(&expression)) {
        runtime::Value lhs;
        runtime::Value rhs;

        if (!EvaluateExpression(*binary->lhs, &lhs, out_error)) {
            return false;
        }
        if (!EvaluateExpression(*binary->rhs, &rhs, out_error)) {
            return false;
        }

        return EvaluateBinary(binary->op, lhs, rhs, out_value, out_error);
    }

    *out_error = "Expresion no soportada por el interprete.";
    return false;
}

bool Interpreter::EvaluateUnary(frontend::UnaryOp op, const runtime::Value& operand, runtime::Value* out_value,
                                std::string* out_error) const {
    if (op == frontend::UnaryOp::LogicalNot) {
        *out_value = runtime::Value(!operand.AsBool());
        return true;
    }

    BigInt integer;
    if (operand.AsBigInt(&integer)) {
        if (op == frontend::UnaryOp::Negate) {
            *out_value = runtime::Value(-integer);
            return true;
        }

        *out_value = runtime::Value(std::move(integer));
        return true;
    }

    if (operand.IsDecimal()) {
        runtime::Value::Decimal decimal;
        if (!operand.AsDecimal(&decimal)) {
            *out_error = "La expresion requiere un valor numerico.";
            return false;
        }

        if (op == frontend::UnaryOp::Negate) {
            *out_value = runtime::Value(runtime::Value::Decimal::FromBigInt(0) - decimal);
            return true;
        }

        *out_value = runtime::Value(decimal);
        return true;
    }

    double numeric = 0.0;
    if (!ReadNumeric(operand, &numeric, out_error)) {
        return false;
    }

    if (op == frontend::UnaryOp::Negate) {
        *out_value = runtime::Value(-numeric);
        return true;
    }

    *out_value = runtime::Value(numeric);
    return true;
}

bool Interpreter::EvaluateBinary(frontend::BinaryOp op, const runtime::Value& lhs, const runtime::Value& rhs,
                                 runtime::Value* out_value, std::string* out_error) const {
    if (op == frontend::BinaryOp::Add && (lhs.IsString() || rhs.IsString())) {
        *out_value = runtime::Value(lhs.ToString() + rhs.ToString());
        return true;
    }

    if (op == frontend::BinaryOp::LogicalAnd) {
        *out_value = runtime::Value(lhs.AsBool() && rhs.AsBool());
        return true;
    }

    if (op == frontend::BinaryOp::LogicalOr) {
        *out_value = runtime::Value(lhs.AsBool() || rhs.AsBool());
        return true;
    }

    if (op == frontend::BinaryOp::Equal || op == frontend::BinaryOp::NotEqual) {
        const bool result = lhs.Equals(rhs);
        *out_value = runtime::Value(op == frontend::BinaryOp::Equal ? result : !result);
        return true;
    }

    if (op == frontend::BinaryOp::In) {
        bool contains = false;

        if (const auto* list = rhs.AsList()) {
            for (const auto& element : *list) {
                if (element.Equals(lhs)) {
                    contains = true;
                    break;
                }
            }
            *out_value = runtime::Value(contains);
            return true;
        }

        if (const auto* tuple = rhs.AsTuple()) {
            for (const auto& element : *tuple) {
                if (element.Equals(lhs)) {
                    contains = true;
                    break;
                }
            }
            *out_value = runtime::Value(contains);
            return true;
        }

        if (const auto* set = rhs.AsSet()) {
            for (const auto& element : *set) {
                if (element.Equals(lhs)) {
                    contains = true;
                    break;
                }
            }
            *out_value = runtime::Value(contains);
            return true;
        }

        if (const auto* map = rhs.AsMap()) {
            for (const auto& entry : *map) {
                if (entry.first.Equals(lhs)) {
                    contains = true;
                    break;
                }
            }
            *out_value = runtime::Value(contains);
            return true;
        }

        if (const auto* object = rhs.AsObject()) {
            const std::string key = lhs.ToString();
            for (const auto& entry : *object) {
                if (entry.first == key) {
                    contains = true;
                    break;
                }
            }
            *out_value = runtime::Value(contains);
            return true;
        }

        if (rhs.IsString()) {
            const std::string haystack = rhs.ToString();
            const std::string needle = lhs.ToString();
            *out_value = runtime::Value(haystack.find(needle) != std::string::npos);
            return true;
        }

        *out_error = "Operador 'in' requiere list, tuple, set, map, object o string a la derecha.";
        return false;
    }

    runtime::Value::Decimal left_decimal;
    runtime::Value::Decimal right_decimal;
    const bool use_decimal =
        (lhs.IsDecimal() || rhs.IsDecimal()) &&
        lhs.AsDecimal(&left_decimal) &&
        rhs.AsDecimal(&right_decimal);

    if (use_decimal) {
        switch (op) {
        case frontend::BinaryOp::Add:
            *out_value = runtime::Value(left_decimal + right_decimal);
            return true;
        case frontend::BinaryOp::Subtract:
            *out_value = runtime::Value(left_decimal - right_decimal);
            return true;
        case frontend::BinaryOp::Multiply:
            *out_value = runtime::Value(left_decimal * right_decimal);
            return true;
        case frontend::BinaryOp::Divide: {
            runtime::Value::Decimal divided;
            if (!runtime::Value::Decimal::Divide(left_decimal, right_decimal, 18, &divided, out_error)) {
                return false;
            }
            *out_value = runtime::Value(std::move(divided));
            return true;
        }
        case frontend::BinaryOp::Power: {
            BigInt exponent_integer;
            if (rhs.AsBigInt(&exponent_integer) && exponent_integer >= 0) {
                long long exponent = 0;
                if (!runtime::Value::TryBigIntToInt64(exponent_integer, &exponent) || exponent > 1000000LL) {
                    *out_error = "pow() exponente entero demasiado grande para computar.";
                    return false;
                }

                runtime::Value::Decimal result = runtime::Value::Decimal::FromBigInt(1);
                runtime::Value::Decimal power = left_decimal;
                unsigned long long exp = static_cast<unsigned long long>(exponent);
                while (exp > 0) {
                    if ((exp & 1ULL) != 0ULL) {
                        result = result * power;
                    }
                    exp >>= 1ULL;
                    if (exp != 0ULL) {
                        power = power * power;
                    }
                }
                *out_value = runtime::Value(std::move(result));
                return true;
            }

            double left_number = 0.0;
            double right_number = 0.0;
            if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
                return false;
            }
            *out_value = runtime::Value(std::pow(left_number, right_number));
            return true;
        }
        case frontend::BinaryOp::Modulo: {
            double left_number = 0.0;
            double right_number = 0.0;
            if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
                return false;
            }
            if (right_number == 0.0) {
                *out_error = "Modulo por cero.";
                return false;
            }
            *out_value = runtime::Value(std::fmod(left_number, right_number));
            return true;
        }
        case frontend::BinaryOp::Less:
            *out_value = runtime::Value(left_decimal < right_decimal);
            return true;
        case frontend::BinaryOp::LessEqual:
            *out_value = runtime::Value(left_decimal <= right_decimal);
            return true;
        case frontend::BinaryOp::Greater:
            *out_value = runtime::Value(left_decimal > right_decimal);
            return true;
        case frontend::BinaryOp::GreaterEqual:
            *out_value = runtime::Value(left_decimal >= right_decimal);
            return true;
        case frontend::BinaryOp::Equal:
        case frontend::BinaryOp::NotEqual:
        case frontend::BinaryOp::In:
        case frontend::BinaryOp::LogicalAnd:
        case frontend::BinaryOp::LogicalOr:
            break;
        }
    }

    BigInt left_integer;
    BigInt right_integer;
    const bool left_is_integer = lhs.AsBigInt(&left_integer);
    const bool right_is_integer = rhs.AsBigInt(&right_integer);

    if (left_is_integer && right_is_integer) {
        switch (op) {
        case frontend::BinaryOp::Add:
            *out_value = runtime::Value(left_integer + right_integer);
            return true;
        case frontend::BinaryOp::Subtract:
            *out_value = runtime::Value(left_integer - right_integer);
            return true;
        case frontend::BinaryOp::Multiply:
            *out_value = runtime::Value(left_integer * right_integer);
            return true;
        case frontend::BinaryOp::Divide: {
            if (right_integer == 0) {
                *out_error = "Division por cero.";
                return false;
            }

            double left_number = 0.0;
            double right_number = 0.0;
            if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
                *out_error = "No se puede convertir entero grande a double para division.";
                return false;
            }
            *out_value = runtime::Value(left_number / right_number);
            return true;
        }
        case frontend::BinaryOp::Modulo:
            if (right_integer == 0) {
                *out_error = "Modulo por cero.";
                return false;
            }
            *out_value = runtime::Value(left_integer % right_integer);
            return true;
        case frontend::BinaryOp::Power: {
            if (right_integer < 0) {
                double left_number = 0.0;
                double right_number = 0.0;
                if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
                    *out_error = "No se puede convertir entero grande a double para potencia.";
                    return false;
                }
                *out_value = runtime::Value(std::pow(left_number, right_number));
                return true;
            }

            BigInt result;
            if (!PowBigInt(left_integer, right_integer, &result, out_error)) {
                return false;
            }
            *out_value = runtime::Value(std::move(result));
            return true;
        }
        case frontend::BinaryOp::Less:
            *out_value = runtime::Value(left_integer < right_integer);
            return true;
        case frontend::BinaryOp::LessEqual:
            *out_value = runtime::Value(left_integer <= right_integer);
            return true;
        case frontend::BinaryOp::Greater:
            *out_value = runtime::Value(left_integer > right_integer);
            return true;
        case frontend::BinaryOp::GreaterEqual:
            *out_value = runtime::Value(left_integer >= right_integer);
            return true;
        case frontend::BinaryOp::Equal:
        case frontend::BinaryOp::NotEqual:
        case frontend::BinaryOp::In:
        case frontend::BinaryOp::LogicalAnd:
        case frontend::BinaryOp::LogicalOr:
            break;
        }
    }

    double left_number = 0.0;
    double right_number = 0.0;
    if (!ReadNumeric(lhs, &left_number, out_error) || !ReadNumeric(rhs, &right_number, out_error)) {
        return false;
    }

    switch (op) {
    case frontend::BinaryOp::Add:
        *out_value = runtime::Value(left_number + right_number);
        return true;
    case frontend::BinaryOp::Subtract:
        *out_value = runtime::Value(left_number - right_number);
        return true;
    case frontend::BinaryOp::Multiply:
        *out_value = runtime::Value(left_number * right_number);
        return true;
    case frontend::BinaryOp::Divide:
        if (right_number == 0.0) {
            *out_error = "Division por cero.";
            return false;
        }
        *out_value = runtime::Value(left_number / right_number);
        return true;
    case frontend::BinaryOp::Modulo:
        if (right_number == 0.0) {
            *out_error = "Modulo por cero.";
            return false;
        }
        *out_value = runtime::Value(std::fmod(left_number, right_number));
        return true;
    case frontend::BinaryOp::Power:
        *out_value = runtime::Value(std::pow(left_number, right_number));
        return true;
    case frontend::BinaryOp::Less:
        *out_value = runtime::Value(left_number < right_number);
        return true;
    case frontend::BinaryOp::LessEqual:
        *out_value = runtime::Value(left_number <= right_number);
        return true;
    case frontend::BinaryOp::Greater:
        *out_value = runtime::Value(left_number > right_number);
        return true;
    case frontend::BinaryOp::GreaterEqual:
        *out_value = runtime::Value(left_number >= right_number);
        return true;
    case frontend::BinaryOp::Equal:
    case frontend::BinaryOp::NotEqual:
    case frontend::BinaryOp::In:
    case frontend::BinaryOp::LogicalAnd:
    case frontend::BinaryOp::LogicalOr:
        break;
    }

    *out_error = "Operacion binaria no soportada.";
    return false;
}

bool Interpreter::ExecuteCall(const frontend::CallExpr& call, bool require_return_value, runtime::Value* out_value,
                              std::string* out_error) {
    bool was_builtin = false;
    if (!ExecuteBuiltinCall(call, &was_builtin, out_value, out_error)) {
        return false;
    }
    if (was_builtin) {
        return true;
    }

    if (call.callee == "super") {
        return ExecuteSuperCall(call, require_return_value, out_value, out_error);
    }

    const std::size_t last_dot = call.callee.rfind('.');
    if (last_dot != std::string::npos && last_dot > 0 && last_dot + 1 < call.callee.size()) {
        const std::string target_name = call.callee.substr(0, last_dot);
        const std::string member_name = call.callee.substr(last_dot + 1);

        if (target_name == "super") {
            if (class_execution_stack_.empty()) {
                *out_error = "super.metodo(...) solo se permite dentro de metodos de clase.";
                return false;
            }

            const std::string current_class = class_execution_stack_.back();
            const frontend::ClassDeclStmt* current_decl = FindClass(current_class);
            if (current_decl == nullptr) {
                *out_error = "Clase no definida: " + current_class;
                return false;
            }
            if (current_decl->base_class.empty()) {
                *out_error = "La clase '" + current_class + "' no tiene clase base para super.";
                return false;
            }

            auto this_it = environment_.find("this");
            if (this_it == environment_.end()) {
                *out_error = "super.metodo(...) requiere contexto de instancia valido.";
                return false;
            }
            std::string this_class;
            if (!IsClassInstance(this_it->second.value, &this_class)) {
                *out_error = "super.metodo(...) requiere contexto de instancia valido.";
                return false;
            }

            const frontend::ClassDeclStmt* base_decl = FindClass(current_decl->base_class);
            if (base_decl == nullptr) {
                *out_error = "Clase base no definida: " + current_decl->base_class;
                return false;
            }

            const frontend::ClassMethodDecl* method = nullptr;
            std::string owner_class;
            if (!ResolveClassMethod(*base_decl, member_name, &method, &owner_class)) {
                *out_error = "Metodo no definido en super: " + member_name;
                return false;
            }
            if (method->is_static) {
                *out_error = "super.metodo(...) solo aplica a metodos de instancia.";
                return false;
            }
            if (!CanAccessMember(method->visibility, owner_class)) {
                *out_error = "Metodo no accesible por visibilidad: " + call.callee;
                return false;
            }

            runtime::Value bound_instance = this_it->second.value;
            if (!ExecuteClassCallable(
                owner_class,
                call.callee,
                method->return_type,
                method->return_annotation,
                method->params,
                method->body,
                call,
                require_return_value,
                out_value,
                out_error,
                &bound_instance,
                false)) {
                return false;
            }

            const auto this_after_call = environment_.find("this");
            if (this_after_call != environment_.end()) {
                this_after_call->second.value = std::move(bound_instance);
            }
            return true;
        }

        std::string resolved_static_target = target_name;
        const auto class_alias_target = class_aliases_.find(target_name);
        if (class_alias_target != class_aliases_.end()) {
            resolved_static_target = class_alias_target->second;
        }

        if (const frontend::ClassDeclStmt* static_class = FindClass(resolved_static_target)) {
            const frontend::ClassMethodDecl* method = nullptr;
            std::string owner_class;
            if (!ResolveClassMethod(*static_class, member_name, &method, &owner_class)) {
                *out_error = "Metodo no definido: " + call.callee;
                return false;
            }
            if (!method->is_static) {
                *out_error = "Metodo de instancia requiere objeto: " + call.callee;
                return false;
            }
            if (!CanAccessMember(method->visibility, owner_class)) {
                *out_error = "Metodo no accesible por visibilidad: " + call.callee;
                return false;
            }

            return ExecuteClassCallable(
                owner_class,
                call.callee,
                method->return_type,
                method->return_annotation,
                method->params,
                method->body,
                call,
                require_return_value,
                out_value,
                out_error,
                nullptr,
                false);
        }

        runtime::Value* target_object = nullptr;
        if (!ResolveMutableVariable(target_name, false, &target_object, out_error)) {
            return false;
        }

        std::string class_name;
        if (!IsClassInstance(*target_object, &class_name)) {
            const auto dotted_class_alias = class_aliases_.find(target_name + "." + member_name);
            if (dotted_class_alias != class_aliases_.end()) {
                const frontend::ClassDeclStmt* class_decl = FindClass(dotted_class_alias->second);
                if (class_decl == nullptr) {
                    *out_error = "Clase no definida: " + dotted_class_alias->second;
                    return false;
                }
                return InstantiateClass(*class_decl, call, out_value, out_error);
            }

            if (!target_object->IsObject()) {
                *out_error = "Llamada de metodo requiere instancia de clase: " + call.callee;
                return false;
            }

            const runtime::Value* module_member = target_object->GetObjectProperty(member_name);
            if (module_member == nullptr) {
                *out_error = "Propiedad no encontrada: " + member_name;
                return false;
            }

            if (const auto* function_ref = module_member->AsFunctionRefValue()) {
                const auto function_it = functions_.find(function_ref->name);
                if (function_it == functions_.end()) {
                    *out_error = "Funcion no definida: " + function_ref->name;
                    return false;
                }
                return ExecuteUserFunction(*function_it->second, call, require_return_value, out_value, out_error);
            }

            if (module_member->IsString()) {
                const std::string class_from_member = module_member->ToString();
                if (const frontend::ClassDeclStmt* member_class = FindClass(class_from_member)) {
                    return InstantiateClass(*member_class, call, out_value, out_error);
                }
            }

            *out_error = "Miembro no invocable: " + call.callee;
            return false;
        }

        const frontend::ClassDeclStmt* class_decl = FindClass(class_name);
        if (class_decl == nullptr) {
            *out_error = "Clase no definida para instancia: " + class_name;
            return false;
        }

        const frontend::ClassMethodDecl* method = nullptr;
        std::string owner_class;
        if (!ResolveClassMethod(*class_decl, member_name, &method, &owner_class)) {
            *out_error = "Metodo no definido: " + call.callee;
            return false;
        }

        if (method->is_static) {
            *out_error = "Metodo static debe invocarse por nombre de clase: " + call.callee;
            return false;
        }

        if (!CanAccessMember(method->visibility, owner_class)) {
            *out_error = "Metodo no accesible por visibilidad: " + call.callee;
            return false;
        }

        runtime::Value bound_instance = *target_object;
        if (!ExecuteClassCallable(
            owner_class,
            call.callee,
            method->return_type,
            method->return_annotation,
            method->params,
            method->body,
            call,
            require_return_value,
            out_value,
            out_error,
            &bound_instance,
            false)) {
            return false;
        }

        runtime::Value* target_after_call = nullptr;
        if (!ResolveMutableVariable(target_name, false, &target_after_call, out_error)) {
            return false;
        }
        *target_after_call = std::move(bound_instance);
        return true;
    }

    std::string resolved_constructor_name = call.callee;
    const auto class_alias = class_aliases_.find(call.callee);
    if (class_alias != class_aliases_.end()) {
        resolved_constructor_name = class_alias->second;
    }

    if (const frontend::ClassDeclStmt* class_decl = FindClass(resolved_constructor_name)) {
        return InstantiateClass(*class_decl, call, out_value, out_error);
    }

    std::string target_function = call.callee;
    const auto variable_it = environment_.find(call.callee);
    if (variable_it != environment_.end()) {
        const auto* function_ref = variable_it->second.value.AsFunctionRefValue();
        if (function_ref != nullptr && !function_ref->name.empty()) {
            target_function = function_ref->name;
        }
    }

    const auto function_it = functions_.find(target_function);
    if (function_it == functions_.end()) {
        *out_error = "Funcion no definida: " + target_function;
        return false;
    }

    return ExecuteUserFunction(*function_it->second, call, require_return_value, out_value, out_error);
}

bool Interpreter::ExecuteClassCallable(const std::string& class_name,
                                       const std::string& callable_name,
                                       frontend::TypeHint return_type,
                                       const frontend::TypeAnnotation& return_annotation,
                                       const std::vector<frontend::FunctionParam>& params,
                                       const std::vector<std::unique_ptr<frontend::Statement>>& body,
                                       const frontend::CallExpr& call,
                                       bool require_return_value,
                                       runtime::Value* out_value,
                                       std::string* out_error,
                                       runtime::Value* bound_this,
                                       bool is_constructor,
                                       bool* out_constructor_called_super) {
    class_execution_stack_.push_back(class_name);
    if (is_constructor) {
        constructor_execution_stack_.push_back(class_name);
        constructor_super_called_stack_.push_back(false);
        constructor_instance_stack_.push_back(bound_this);
    }

    const bool ok = ExecuteCallable(
        callable_name,
        return_type,
        return_annotation,
        params,
        body,
        call,
        require_return_value,
        out_value,
        out_error,
        bound_this);

    if (is_constructor && !constructor_execution_stack_.empty()) {
        if (out_constructor_called_super != nullptr && !constructor_super_called_stack_.empty()) {
            *out_constructor_called_super = constructor_super_called_stack_.back();
        }
        if (!constructor_super_called_stack_.empty()) {
            constructor_super_called_stack_.pop_back();
        }
        if (!constructor_instance_stack_.empty()) {
            constructor_instance_stack_.pop_back();
        }
        constructor_execution_stack_.pop_back();
    }
    if (!class_execution_stack_.empty()) {
        class_execution_stack_.pop_back();
    }
    return ok;
}

bool Interpreter::ExecuteCallable(const std::string& callable_name,
                                  frontend::TypeHint return_type,
                                  const frontend::TypeAnnotation& return_annotation,
                                  const std::vector<frontend::FunctionParam>& params,
                                  const std::vector<std::unique_ptr<frontend::Statement>>& body,
                                  const frontend::CallExpr& call,
                                  bool require_return_value,
                                  runtime::Value* out_value,
                                  std::string* out_error,
                                  runtime::Value* bound_this) {
    if (call.arguments.size() > params.size()) {
        *out_error = "Numero incorrecto de argumentos para funcion '" + callable_name + "'.";
        return false;
    }

    struct RefBinding {
        std::string param;
        std::string caller;
        frontend::TypeAnnotation type_annotation;
    };

    std::vector<RefBinding> refs;
    auto caller_environment = environment_;
    auto local_environment = environment_;

    if (bound_this != nullptr) {
        local_environment["this"] = runtime::VariableSlot{*bound_this, runtime::VariableKind::Dynamic};
    }

    for (std::size_t i = 0; i < params.size(); ++i) {
        const frontend::FunctionParam& param = params[i];
        const frontend::TypeAnnotation param_annotation =
            EffectiveTypeAnnotation(param.type_annotation, param.type_hint);
        const bool param_has_annotation =
            param_annotation.base != frontend::TypeHint::Inferred || !param_annotation.type_args.empty();
        const bool has_argument = i < call.arguments.size();
        if (!has_argument && param.default_value == nullptr) {
            *out_error = "Numero incorrecto de argumentos para funcion '" + callable_name + "'.";
            return false;
        }

        if (param.by_reference) {
            if (!has_argument) {
                *out_error = "Parametro por referencia '" + param.name + "' requiere argumento explicito.";
                return false;
            }

            const frontend::CallArgument& argument = call.arguments[i];
            const auto* variable = dynamic_cast<const frontend::VariableExpr*>(argument.value.get());
            if (variable == nullptr) {
                *out_error = "Parametro por referencia '" + param.name + "' requiere una variable.";
                return false;
            }

            if (variable->name.find('.') != std::string::npos) {
                *out_error = "Referencia no soporta acceso con propiedad: " + variable->name;
                return false;
            }

            const auto caller_it = caller_environment.find(variable->name);
            if (caller_it == caller_environment.end()) {
                *out_error = "Variable no definida para referencia: " + variable->name;
                return false;
            }

            runtime::VariableSlot reference_slot = caller_it->second;
            if (param_has_annotation) {
                runtime::Value normalized;
                std::string type_error;
                if (!NormalizeValueForTypeAnnotation(param_annotation, reference_slot.value, &normalized, &type_error)) {
                    *out_error =
                        "Argumento por referencia '" + param.name +
                        "' no coincide con type hint '" + TypeAnnotationName(param_annotation) + "': " + type_error;
                    return false;
                }
                reference_slot.value = std::move(normalized);
            }

            local_environment[param.name] = std::move(reference_slot);
            refs.push_back(RefBinding{param.name, variable->name, param_annotation});
            continue;
        }

        runtime::Value evaluated;
        if (has_argument) {
            const frontend::CallArgument& argument = call.arguments[i];
            if (argument.by_reference) {
                *out_error = "No se puede pasar '&' a un parametro por valor: " + param.name;
                return false;
            }

            if (!EvaluateExpression(*argument.value, &evaluated, out_error)) {
                return false;
            }
        } else {
            if (param.default_value == nullptr) {
                *out_error = "Error interno: parametro sin argumento ni default en '" + callable_name + "'.";
                return false;
            }

            if (!EvaluateExpression(*param.default_value, &evaluated, out_error)) {
                return false;
            }
        }

        if (param_has_annotation) {
            runtime::Value normalized;
            std::string type_error;
            if (!NormalizeValueForTypeAnnotation(param_annotation, evaluated, &normalized, &type_error)) {
                *out_error =
                    "Argumento '" + param.name +
                    "' no coincide con type hint '" + TypeAnnotationName(param_annotation) + "': " + type_error;
                return false;
            }
            evaluated = std::move(normalized);
        }

        local_environment[param.name] = runtime::VariableSlot{evaluated, runtime::VariableKind::Dynamic};
    }

    environment_ = std::move(local_environment);
    return_stack_.push_back(std::nullopt);

    if (!ExecuteBlock(body, out_error)) {
        return_stack_.pop_back();
        environment_ = std::move(caller_environment);
        return false;
    }

    std::optional<runtime::Value> returned = return_stack_.back();
    return_stack_.pop_back();

    if (bound_this != nullptr) {
        const auto this_it = environment_.find("this");
        if (this_it != environment_.end()) {
            *bound_this = this_it->second.value;
        }
    }

    const frontend::TypeAnnotation effective_return_annotation =
        EffectiveTypeAnnotation(return_annotation, return_type);
    const bool has_return_annotation =
        effective_return_annotation.base != frontend::TypeHint::Inferred ||
        !effective_return_annotation.type_args.empty();
    if (has_return_annotation) {
        if (!returned.has_value()) {
            *out_error =
                "La funcion '" + callable_name +
                "' debe retornar un valor de tipo '" + TypeAnnotationName(effective_return_annotation) + "'.";
            environment_ = std::move(caller_environment);
            return false;
        }

        runtime::Value normalized_return;
        std::string type_error;
        if (!NormalizeValueForTypeAnnotation(
                effective_return_annotation,
                *returned,
                &normalized_return,
                &type_error)) {
            *out_error =
                "El retorno de la funcion '" + callable_name +
                "' no coincide con type hint '" + TypeAnnotationName(effective_return_annotation) + "': " + type_error;
            environment_ = std::move(caller_environment);
            return false;
        }
        returned = std::move(normalized_return);
    }

    for (const RefBinding& ref : refs) {
        const auto updated = environment_.find(ref.param);
        if (updated != environment_.end()) {
            runtime::VariableSlot propagated_slot = updated->second;
            const bool ref_has_annotation =
                ref.type_annotation.base != frontend::TypeHint::Inferred ||
                !ref.type_annotation.type_args.empty();
            if (ref_has_annotation) {
                runtime::Value normalized;
                std::string type_error;
                if (!NormalizeValueForTypeAnnotation(
                        ref.type_annotation,
                        propagated_slot.value,
                        &normalized,
                        &type_error)) {
                    *out_error =
                        "El valor final del parametro por referencia '" + ref.param +
                        "' no coincide con type hint '" + TypeAnnotationName(ref.type_annotation) + "': " + type_error;
                    environment_ = std::move(caller_environment);
                    return false;
                }
                propagated_slot.value = std::move(normalized);
            }
            caller_environment[ref.caller] = std::move(propagated_slot);
        }
    }

    environment_ = std::move(caller_environment);

    if (require_return_value) {
        if (out_value != nullptr) {
            *out_value = returned.has_value() ? *returned : runtime::Value(nullptr);
        }
        return true;
    }

    if (out_value != nullptr) {
        *out_value = returned.has_value() ? *returned : runtime::Value(nullptr);
    }

    return true;
}

bool Interpreter::ExecuteUserFunction(const frontend::FunctionDeclStmt& function, const frontend::CallExpr& call,
                                      bool require_return_value, runtime::Value* out_value, std::string* out_error) {
    return ExecuteCallable(
        function.name,
        function.return_type,
        function.return_annotation,
        function.params,
        function.body,
        call,
        require_return_value,
        out_value,
        out_error,
        nullptr);
}

bool Interpreter::ExecuteSuperCall(const frontend::CallExpr& call, bool require_return_value, runtime::Value* out_value,
                                   std::string* out_error) {
    (void)require_return_value;

    if (constructor_execution_stack_.empty()) {
        *out_error = "super(...) solo se permite dentro de constructor de clase.";
        return false;
    }
    if (constructor_instance_stack_.empty() || constructor_super_called_stack_.empty()) {
        *out_error = "Error interno: estado invalido para super(...).";
        return false;
    }

    if (constructor_super_called_stack_.back()) {
        *out_error = "super(...) solo puede invocarse una vez por constructor.";
        return false;
    }

    runtime::Value* instance = constructor_instance_stack_.back();
    if (instance == nullptr || !instance->IsObject()) {
        *out_error = "Error interno: instancia invalida en super(...).";
        return false;
    }

    const std::string current_class = constructor_execution_stack_.back();
    const frontend::ClassDeclStmt* current_decl = FindClass(current_class);
    if (current_decl == nullptr) {
        *out_error = "Clase no definida: " + current_class;
        return false;
    }
    if (current_decl->base_class.empty()) {
        *out_error = "super(...) requiere una clase base en '" + current_class + "'.";
        return false;
    }

    const frontend::ClassDeclStmt* base_decl = FindClass(current_decl->base_class);
    if (base_decl == nullptr) {
        *out_error = "Clase base no definida: " + current_decl->base_class;
        return false;
    }

    if (!ExecuteClassConstructor(*base_decl, instance, call, out_error)) {
        return false;
    }

    auto this_it = environment_.find("this");
    if (this_it != environment_.end()) {
        this_it->second.value = *instance;
    }

    constructor_super_called_stack_.back() = true;
    if (out_value != nullptr) {
        *out_value = runtime::Value(nullptr);
    }
    return true;
}

} // namespace clot::interpreter
