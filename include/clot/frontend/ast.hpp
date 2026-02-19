#ifndef CLOT_FRONTEND_AST_HPP
#define CLOT_FRONTEND_AST_HPP

#include <memory>
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
    Long,
    Byte,
};

struct Expr {
    virtual ~Expr() = default;
};

struct NumberExpr final : Expr {
    explicit NumberExpr(double in_value) : value(in_value) {}
    double value = 0.0;
};

struct StringExpr final : Expr {
    explicit StringExpr(std::string in_value) : value(std::move(in_value)) {}
    std::string value;
};

struct BoolExpr final : Expr {
    explicit BoolExpr(bool in_value) : value(in_value) {}
    bool value = false;
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
        std::unique_ptr<Expr> in_expr)
        : name(std::move(in_name)),
          op(in_op),
          declaration_type(in_decl_type),
          expr(std::move(in_expr)) {}

    std::string name;
    AssignmentOp op = AssignmentOp::Set;
    DeclarationType declaration_type = DeclarationType::Inferred;
    std::unique_ptr<Expr> expr;
};

struct PrintStmt final : Statement {
    explicit PrintStmt(std::unique_ptr<Expr> in_expr) : expr(std::move(in_expr)) {}
    std::unique_ptr<Expr> expr;
};

struct IfStmt final : Statement {
    explicit IfStmt(std::unique_ptr<Expr> in_condition) : condition(std::move(in_condition)) {}

    std::unique_ptr<Expr> condition;
    std::vector<std::unique_ptr<Statement>> then_branch;
    std::vector<std::unique_ptr<Statement>> else_branch;
};

struct FunctionParam {
    std::string name;
    bool by_reference = false;
};

struct FunctionDeclStmt final : Statement {
    FunctionDeclStmt(
        std::string in_name,
        std::vector<FunctionParam> in_params,
        std::vector<std::unique_ptr<Statement>> in_body)
        : name(std::move(in_name)),
          params(std::move(in_params)),
          body(std::move(in_body)) {}

    std::string name;
    std::vector<FunctionParam> params;
    std::vector<std::unique_ptr<Statement>> body;
};

struct ImportStmt final : Statement {
    explicit ImportStmt(std::string in_module) : module_name(std::move(in_module)) {}
    std::string module_name;
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
