#ifndef CLOT_FRONTEND_AST_HPP
#define CLOT_FRONTEND_AST_HPP

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace clot::frontend {

enum class BinaryOp {
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    Power,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    In,
    LogicalAnd,
    LogicalOr,
};

enum class UnaryOp {
    Plus,
    Negate,
    LogicalNot,
};

enum class AssignmentOp {
    Set,
    AddAssign,
    SubAssign,
};

enum class DeclarationType {
    Inferred,
    Int,
    Double,
    Float,
    Decimal,
    Long,
    Byte,
    Char,
    Tuple,
    Set,
    Map,
    Function,
};

enum class TypeHint {
    Inferred,
    Int,
    Double,
    Float,
    Decimal,
    Long,
    Byte,
    Char,
    Tuple,
    Set,
    Map,
    Function,
    String,
    Bool,
    List,
    Object,
    Null,
};

enum class MemberVisibility {
    Public,
    Private,
};

struct Expr {
    virtual ~Expr() = default;
};

struct NumberExpr final : Expr {
    NumberExpr(
        double in_value,
        std::string in_lexeme,
        bool in_is_integer_literal,
        std::optional<long long> in_exact_integer64)
        : value(in_value),
          lexeme(std::move(in_lexeme)),
          is_integer_literal(in_is_integer_literal),
          exact_integer64(std::move(in_exact_integer64)) {}

    double value = 0.0;
    std::string lexeme;
    bool is_integer_literal = false;
    std::optional<long long> exact_integer64;
};

struct StringExpr final : Expr {
    explicit StringExpr(std::string in_value) : value(std::move(in_value)) {}
    std::string value;
};

struct BoolExpr final : Expr {
    explicit BoolExpr(bool in_value) : value(in_value) {}
    bool value = false;
};

struct CharExpr final : Expr {
    explicit CharExpr(char in_value) : value(in_value) {}
    char value = '\0';
};

struct NullExpr final : Expr {
};

struct VariableExpr final : Expr {
    explicit VariableExpr(std::string in_name) : name(std::move(in_name)) {}
    std::string name;
};

struct ListExpr final : Expr {
    explicit ListExpr(std::vector<std::unique_ptr<Expr>> in_elements)
        : elements(std::move(in_elements)) {}

    std::vector<std::unique_ptr<Expr>> elements;
};

struct ObjectEntryExpr {
    std::string key;
    std::unique_ptr<Expr> value;
};

struct ObjectExpr final : Expr {
    explicit ObjectExpr(std::vector<ObjectEntryExpr> in_entries)
        : entries(std::move(in_entries)) {}

    std::vector<ObjectEntryExpr> entries;
};

struct IndexExpr final : Expr {
    IndexExpr(std::unique_ptr<Expr> in_collection, std::unique_ptr<Expr> in_index)
        : collection(std::move(in_collection)), index(std::move(in_index)) {}

    std::unique_ptr<Expr> collection;
    std::unique_ptr<Expr> index;
};

struct CallArgument {
    bool by_reference = false;
    std::unique_ptr<Expr> value;
};

struct CallExpr final : Expr {
    CallExpr(std::string in_callee, std::vector<CallArgument> in_arguments)
        : callee(std::move(in_callee)), arguments(std::move(in_arguments)) {}

    std::string callee;
    std::vector<CallArgument> arguments;
};

struct UnaryExpr final : Expr {
    UnaryExpr(UnaryOp in_op, std::unique_ptr<Expr> in_operand)
        : op(in_op), operand(std::move(in_operand)) {}

    UnaryOp op = UnaryOp::Plus;
    std::unique_ptr<Expr> operand;
};

struct BinaryExpr final : Expr {
    BinaryExpr(BinaryOp in_op, std::unique_ptr<Expr> in_lhs, std::unique_ptr<Expr> in_rhs)
        : op(in_op), lhs(std::move(in_lhs)), rhs(std::move(in_rhs)) {}

    BinaryOp op = BinaryOp::Add;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

struct Statement {
    virtual ~Statement() = default;
};

struct AssignmentStmt final : Statement {
    AssignmentStmt(
        std::string in_name,
        AssignmentOp in_op,
        DeclarationType in_decl_type,
        std::unique_ptr<Expr> in_expr,
        bool in_is_const = false)
        : name(std::move(in_name)),
          op(in_op),
          declaration_type(in_decl_type),
          expr(std::move(in_expr)),
          is_const(in_is_const) {}

    std::string name;
    AssignmentOp op = AssignmentOp::Set;
    DeclarationType declaration_type = DeclarationType::Inferred;
    std::unique_ptr<Expr> expr;
    bool is_const = false;
};

struct PrintStmt final : Statement {
    PrintStmt(std::unique_ptr<Expr> in_expr, bool in_append_newline)
        : expr(std::move(in_expr)),
          append_newline(in_append_newline) {}

    std::unique_ptr<Expr> expr;
    bool append_newline = true;
};

struct IfStmt final : Statement {
    explicit IfStmt(std::unique_ptr<Expr> in_condition) : condition(std::move(in_condition)) {}

    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Statement>> then_branch;
    std::vector<std::unique_ptr<Statement>> else_branch;
};

struct TryCatchStmt final : Statement {
    TryCatchStmt(
        std::vector<std::unique_ptr<Statement>> in_try_branch,
        bool in_has_catch,
        std::string in_catch_type,
        std::string in_error_binding,
        std::vector<std::unique_ptr<Statement>> in_catch_branch,
        std::vector<std::unique_ptr<Statement>> in_finally_branch)
        : try_branch(std::move(in_try_branch)),
          has_catch(in_has_catch),
          catch_type(std::move(in_catch_type)),
          error_binding(std::move(in_error_binding)),
          catch_branch(std::move(in_catch_branch)),
          finally_branch(std::move(in_finally_branch)) {}

    std::vector<std::unique_ptr<Statement>> try_branch;
    bool has_catch = true;
    std::string catch_type;
    std::string error_binding;
    std::vector<std::unique_ptr<Statement>> catch_branch;
    std::vector<std::unique_ptr<Statement>> finally_branch;
};

struct WhileStmt final : Statement {
    WhileStmt(
        std::unique_ptr<Expr> in_condition,
        std::vector<std::unique_ptr<Statement>> in_body)
        : condition(std::move(in_condition)),
          body(std::move(in_body)) {}

    // Condición que evalúa antes de cada iter. 
    std::unique_ptr<Expr> condition;

    // Se ejecutan las sentencias si la condición es verdadera. 
    std::vector<std::unique_ptr<Statement>> body;
};

struct ForStmt final : Statement {
    ForStmt(
        std::unique_ptr<Statement> in_initializer,
        std::unique_ptr<Expr> in_condition,
        std::unique_ptr<Statement> in_update,
        std::vector<std::unique_ptr<Statement>> in_body)
        : initializer(std::move(in_initializer)),
          condition(std::move(in_condition)),
          update(std::move(in_update)),
          body(std::move(in_body)) {}

    std::unique_ptr<Statement> initializer;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Statement> update;
    std::vector<std::unique_ptr<Statement>> body;
};

struct ForEachStmt final : Statement {
    ForEachStmt(
        std::string in_variable_name,
        DeclarationType in_variable_type,
        bool in_variable_is_const,
        std::unique_ptr<Expr> in_collection,
        std::vector<std::unique_ptr<Statement>> in_body)
        : variable_name(std::move(in_variable_name)),
          variable_type(in_variable_type),
          variable_is_const(in_variable_is_const),
          collection(std::move(in_collection)),
          body(std::move(in_body)) {}

    std::string variable_name;
    DeclarationType variable_type = DeclarationType::Inferred;
    bool variable_is_const = false;
    std::unique_ptr<Expr> collection;
    std::vector<std::unique_ptr<Statement>> body;
};

struct DoWhileStmt final : Statement {
    DoWhileStmt(
        std::vector<std::unique_ptr<Statement>> in_body,
        std::unique_ptr<Expr> in_condition)
        : body(std::move(in_body)),
          condition(std::move(in_condition)) {}

    std::vector<std::unique_ptr<Statement>> body;
    std::unique_ptr<Expr> condition;
};

struct SwitchCase {
    std::unique_ptr<Expr> match_expr;
    std::vector<std::unique_ptr<Statement>> body;
    bool is_default = false;
};

struct SwitchStmt final : Statement {
    SwitchStmt(
        std::unique_ptr<Expr> in_value,
        std::vector<SwitchCase> in_cases)
        : value(std::move(in_value)),
          cases(std::move(in_cases)) {}

    std::unique_ptr<Expr> value;
    std::vector<SwitchCase> cases;
};

struct BreakStmt final : Statement {
};

struct ContinueStmt final : Statement {
};

struct PassStmt final : Statement {
};

struct DeferStmt final : Statement {
    explicit DeferStmt(std::unique_ptr<Statement> in_statement)
        : statement(std::move(in_statement)) {}

    std::unique_ptr<Statement> statement;
};

struct FunctionParam {
    std::string name;
    bool by_reference = false;
    TypeHint type_hint = TypeHint::Inferred;
    std::unique_ptr<Expr> default_value;
};

struct FunctionDeclStmt final : Statement {
    FunctionDeclStmt(
        std::string in_name,
        TypeHint in_return_type,
        std::vector<FunctionParam> in_params,
        std::vector<std::unique_ptr<Statement>> in_body)
        : name(std::move(in_name)),
          return_type(in_return_type),
          params(std::move(in_params)),
          body(std::move(in_body)) {}

    std::string name;
    TypeHint return_type = TypeHint::Inferred;
    std::vector<FunctionParam> params;
    std::vector<std::unique_ptr<Statement>> body;
};

struct InterfaceMethodSignature {
    std::string name;
    TypeHint return_type = TypeHint::Inferred;
    std::vector<FunctionParam> params;
};

struct InterfaceDeclStmt final : Statement {
    InterfaceDeclStmt(
        std::string in_name,
        std::vector<InterfaceMethodSignature> in_methods)
        : name(std::move(in_name)),
          methods(std::move(in_methods)) {}

    std::string name;
    std::vector<InterfaceMethodSignature> methods;
};

struct ClassFieldDecl {
    std::string name;
    TypeHint type_hint = TypeHint::Inferred;
    MemberVisibility visibility = MemberVisibility::Public;
    bool is_static = false;
    bool is_readonly = false;
    std::unique_ptr<Expr> default_value;
};

struct ClassMethodDecl {
    std::string name;
    TypeHint return_type = TypeHint::Inferred;
    std::vector<FunctionParam> params;
    std::vector<std::unique_ptr<Statement>> body;
    MemberVisibility visibility = MemberVisibility::Public;
    bool is_static = false;
    bool is_override = false;
};

struct ClassAccessorDecl {
    std::string name;
    bool is_setter = false;
    std::string setter_param_name = "value";
    TypeHint setter_param_type = TypeHint::Inferred;
    std::vector<std::unique_ptr<Statement>> body;
    MemberVisibility visibility = MemberVisibility::Public;
};

struct ClassDeclStmt final : Statement {
    ClassDeclStmt(
        std::string in_name,
        std::string in_base_class,
        std::vector<std::string> in_interfaces,
        std::vector<ClassFieldDecl> in_fields,
        bool in_constructor_is_private,
        std::vector<FunctionParam> in_constructor_params,
        std::vector<std::unique_ptr<Statement>> in_constructor_body,
        std::vector<ClassMethodDecl> in_methods,
        std::vector<ClassAccessorDecl> in_accessors)
        : name(std::move(in_name)),
          base_class(std::move(in_base_class)),
          interfaces(std::move(in_interfaces)),
          fields(std::move(in_fields)),
          constructor_is_private(in_constructor_is_private),
          constructor_params(std::move(in_constructor_params)),
          constructor_body(std::move(in_constructor_body)),
          methods(std::move(in_methods)),
          accessors(std::move(in_accessors)) {}

    std::string name;
    std::string base_class;
    std::vector<std::string> interfaces;
    std::vector<ClassFieldDecl> fields;
    bool constructor_is_private = false;
    std::vector<FunctionParam> constructor_params;
    std::vector<std::unique_ptr<Statement>> constructor_body;
    std::vector<ClassMethodDecl> methods;
    std::vector<ClassAccessorDecl> accessors;
};

struct ImportStmt final : Statement {
    enum class Style {
        Module,
        ModuleAlias,
        FromImport,
    };

    ImportStmt(
        Style in_style,
        std::string in_module,
        std::string in_alias_name,
        std::string in_imported_symbol,
        std::string in_imported_alias)
        : style(in_style),
          module_name(std::move(in_module)),
          alias_name(std::move(in_alias_name)),
          imported_symbol(std::move(in_imported_symbol)),
          imported_alias(std::move(in_imported_alias)) {}

    Style style = Style::Module;
    std::string module_name;
    std::string alias_name;
    std::string imported_symbol;
    std::string imported_alias;
};

struct EnumDeclStmt final : Statement {
    EnumDeclStmt(std::string in_name, std::vector<std::string> in_members)
        : name(std::move(in_name)),
          members(std::move(in_members)) {}

    std::string name;
    std::vector<std::string> members;
};

struct ExpressionStmt final : Statement {
    explicit ExpressionStmt(std::unique_ptr<Expr> in_expr) : expr(std::move(in_expr)) {}
    std::unique_ptr<Expr> expr;
};

struct MutationStmt final : Statement {
    MutationStmt(
        std::unique_ptr<Expr> in_target,
        AssignmentOp in_op,
        std::unique_ptr<Expr> in_expr)
        : target(std::move(in_target)),
          op(in_op),
          expr(std::move(in_expr)) {}

    std::unique_ptr<Expr> target;
    AssignmentOp op = AssignmentOp::Set;
    std::unique_ptr<Expr> expr;
};

struct ReturnStmt final : Statement {
    explicit ReturnStmt(std::unique_ptr<Expr> in_expr) : expr(std::move(in_expr)) {}
    std::unique_ptr<Expr> expr;
};

struct Program {
    std::vector<std::unique_ptr<Statement>> statements;
};

}  // namespace clot::frontend

#endif  // CLOT_FRONTEND_AST_HPP
