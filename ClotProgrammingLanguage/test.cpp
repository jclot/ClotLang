#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

// Token enumerations
enum Token {
    tok_eof = -1,
    tok_var = -2,
    tok_func = -3,
    tok_print = -4,
    tok_identifier = -5,
    tok_number = -6,
};

std::string IdentifierStr; // Filled in if tok_identifier
double NumVal;             // Filled in if tok_number

// Lexer
int gettok() {
    static int LastChar = ' ';

    // Skip any whitespace.
    while (isspace(LastChar))
        LastChar = getchar();

    if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar())))
            IdentifierStr += LastChar;

        if (IdentifierStr == "var") return tok_var;
        if (IdentifierStr == "func") return tok_func;
        if (IdentifierStr == "print") return tok_print;
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9.]+
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    if (LastChar == '#') {
        // Comment until end of line.
        do
            LastChar = getchar();
        while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF)
            return gettok();
    }

    if (LastChar == EOF)
        return tok_eof;

    // Otherwise, just return the character as its ascii value.
    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

// Abstract Syntax Tree (AST) base class
class ExprAST {
public:
    virtual ~ExprAST() = default;
    virtual llvm::Value* codegen() = 0;
};

// Expression class for numeric literals like "1.0"
class NumberExprAST : public ExprAST {
    double Val;

public:
    NumberExprAST(double Val) : Val(Val) {}
    llvm::Value* codegen() override;
};

// Expression class for referencing a variable, like "a"
class VariableExprAST : public ExprAST {
    std::string Name;

public:
    VariableExprAST(const std::string& Name) : Name(Name) {}
    llvm::Value* codegen() override;
};

// Expression class for printing a value
class PrintExprAST : public ExprAST {
    std::unique_ptr<ExprAST> Expr;

public:
    PrintExprAST(std::unique_ptr<ExprAST> Expr) : Expr(std::move(Expr)) {}
    llvm::Value* codegen() override;
};

// This class represents a function definition
class FunctionAST {
    std::string Name;
    std::vector<std::string> Args;
    std::vector<std::unique_ptr<ExprAST>> Body;

public:
    FunctionAST(const std::string& Name, std::vector<std::string> Args,
        std::vector<std::unique_ptr<ExprAST>> Body)
        : Name(Name), Args(std::move(Args)), Body(std::move(Body)) {
    }

    llvm::Function* codegen();
};

// Forward declare
static std::unique_ptr<ExprAST> ParseExpression();

static int CurTok;
static int getNextToken() { return CurTok = gettok(); }

std::unique_ptr<ExprAST> ParseNumberExpr() {
    auto Result = std::make_unique<NumberExprAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

std::unique_ptr<ExprAST> ParseIdentifierExpr() {
    std::string IdName = IdentifierStr;
    getNextToken(); // eat identifier.
    return std::make_unique<VariableExprAST>(IdName);
}

std::unique_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
    default:
        return nullptr;
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_number:
        return ParseNumberExpr();
    }
}

std::unique_ptr<ExprAST> ParseExpression() {
    return ParsePrimary();
}

std::unique_ptr<ExprAST> ParsePrint() {
    getNextToken(); // eat print
    auto Expr = ParseExpression();
    if (!Expr)
        return nullptr;
    return std::make_unique<PrintExprAST>(std::move(Expr));
}

std::unique_ptr<FunctionAST> ParseDefinition() {
    getNextToken(); // eat func
    if (CurTok != tok_identifier)
        return nullptr;

    std::string FnName = IdentifierStr;
    getNextToken(); // eat function name

    std::vector<std::string> ArgNames;
    if (CurTok != '(')
        return nullptr;

    getNextToken(); // eat (
    while (CurTok == tok_identifier) {
        ArgNames.push_back(IdentifierStr);
        getNextToken();
        if (CurTok == ',')
            getNextToken();
    }
    if (CurTok != ')')
        return nullptr;

    getNextToken(); // eat )

    std::vector<std::unique_ptr<ExprAST>> Body;
    while (CurTok != tok_eof) {
        if (CurTok == tok_print) {
            Body.push_back(ParsePrint());
        }
        else if (CurTok == tok_identifier || CurTok == tok_number) {
            Body.push_back(ParseExpression());
        }
        else {
            getNextToken();
        }
    }

    return std::make_unique<FunctionAST>(FnName, std::move(ArgNames), std::move(Body));
}

// LLVM context, builder, and module
llvm::LLVMContext TheContext;
llvm::IRBuilder<> Builder(TheContext);
std::unique_ptr<llvm::Module> TheModule;
std::map<std::string, llvm::Value*> NamedValues;

llvm::Value* LogErrorV(const char* Str) {
    fprintf(stderr, "LogErrorV: %s\n", Str);
    return nullptr;
}

llvm::Value* NumberExprAST::codegen() {
    return llvm::ConstantFP::get(TheContext, llvm::APFloat(Val));
}

llvm::Value* VariableExprAST::codegen() {
    llvm::Value* V = NamedValues[Name];
    if (!V)
        LogErrorV("Unknown variable name");
    return V;
}

llvm::Value* PrintExprAST::codegen() {
    llvm::Value* Val = Expr->codegen();
    if (!Val)
        return nullptr;

    llvm::Function* PrintFunc = TheModule->getFunction("print");
    if (!PrintFunc)
        return LogErrorV("Unknown function referenced");

    return Builder.CreateCall(PrintFunc, Val, "calltmp");
}

llvm::Function* getFunction(const std::string& Name) {
    if (auto* F = TheModule->getFunction(Name))
        return F;

    return nullptr;
}

llvm::Function* FunctionAST::codegen() {
    std::vector<llvm::Type*> Doubles(Args.size(), llvm::Type::getDoubleTy(TheContext));
    llvm::FunctionType* FT = llvm::FunctionType::get(llvm::Type::getDoubleTy(TheContext), Doubles, false);
    llvm::Function* F = llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, TheModule.get());

    unsigned Idx = 0;
    for (auto& Arg : F->args())
        Arg.setName(Args[Idx++]);

    llvm::BasicBlock* BB = llvm::BasicBlock::Create(TheContext, "entry", F);
    Builder.SetInsertPoint(BB);

    NamedValues.clear();
    for (auto& Arg : F->args())
        NamedValues[std::string(Arg.getName())] = &Arg;

    for (auto& Expr : Body) {
        if (!Expr->codegen())
            return nullptr;
    }

    Builder.CreateRet(llvm::ConstantFP::get(TheContext, llvm::APFloat(0.0)));
    llvm::verifyFunction(*F);

    return F;
}

int main() {
    // Initialize LLVM
    TheModule = std::make_unique<llvm::Module>("clotlang", TheContext);

    // Read source code from file
    freopen("test.clot", "r", stdin);

    // Parse the source code
    getNextToken();
    while (CurTok != tok_eof) {
        if (CurTok == tok_func) {
            if (auto FnAST = ParseDefinition()) {
                if (auto* FnIR = FnAST->codegen()) {
                    fprintf(stderr, "Read function definition:");
                    FnIR->print(llvm::errs());
                    fprintf(stderr, "\n");
                }
            }
            else {
                getNextToken();
            }
        }
        else {
            getNextToken();
        }
    }

    // Output the generated code
    TheModule->print(llvm::errs(), nullptr);

    return 0;
}