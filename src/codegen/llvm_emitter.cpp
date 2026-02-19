#include "llvm_backend_internal.hpp"

#ifdef CLOT_HAS_LLVM

#include <optional>
#include <utility>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

namespace clot::codegen::internal {

LlvmEmitter::LlvmEmitter(std::string module_name)
    : module_(std::make_unique<llvm::Module>(module_name, context_)),
      builder_(context_) {}

bool LlvmEmitter::EmitProgram(
    const frontend::Program& program,
    const CompileOptions& options,
    std::string* out_error) {
    use_runtime_bridge_ = !IsAotSupportedProgram(program);

    if (use_runtime_bridge_) {
        if (!EmitRuntimeBridgeProgram(options)) {
            *out_error = error_;
            return false;
        }
        return true;
    }

    math_module_imported_ = false;
    for (const auto& statement : program.statements) {
        if (statement != nullptr && ContainsMathImportInStatement(*statement)) {
            math_module_imported_ = true;
            break;
        }
    }

    if (!DeclareUserFunctions(program)) {
        *out_error = error_;
        return false;
    }

    if (!EmitUserFunctions()) {
        *out_error = error_;
        return false;
    }

    if (!CreateMainFunction()) {
        *out_error = error_;
        return false;
    }

    for (const auto& statement : program.statements) {
        if (statement == nullptr || !EmitStatement(*statement, true)) {
            *out_error = error_;
            return false;
        }
    }

    builder_.CreateRet(llvm::ConstantInt::get(builder_.getInt32Ty(), 0));

    if (llvm::verifyFunction(*main_function_, &llvm::errs())) {
        *out_error = "LLVM genero una funcion main invalida.";
        return false;
    }
    if (llvm::verifyModule(*module_, &llvm::errs())) {
        *out_error = "LLVM genero un modulo invalido.";
        return false;
    }

    return true;
}

bool LlvmEmitter::UsedRuntimeBridge() const {
    return use_runtime_bridge_;
}

bool LlvmEmitter::EmitIRFile(const std::string& output_path, std::string* out_error) {
    std::error_code error_code;
    llvm::raw_fd_ostream output(output_path, error_code, llvm::sys::fs::OF_Text);
    if (error_code) {
        *out_error = "No se pudo escribir IR en '" + output_path + "': " + error_code.message();
        return false;
    }

    module_->print(output, nullptr);
    return true;
}

bool LlvmEmitter::EmitObjectFile(
    const std::string& output_path,
    const std::string& requested_target,
    std::string* out_error) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    const std::string target_triple =
        requested_target.empty() ? llvm::sys::getDefaultTargetTriple() : requested_target;

    std::string target_error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(target_triple, target_error);
    if (target == nullptr) {
        *out_error = "No se encontro target LLVM '" + target_triple + "': " + target_error;
        return false;
    }

    llvm::TargetOptions options;
    std::unique_ptr<llvm::TargetMachine> target_machine(
        target->createTargetMachine(target_triple, "generic", "", options, std::nullopt));

    if (!target_machine) {
        *out_error = "No se pudo crear TargetMachine para '" + target_triple + "'.";
        return false;
    }

    module_->setTargetTriple(target_triple);
    module_->setDataLayout(target_machine->createDataLayout());

    std::error_code error_code;
    llvm::raw_fd_ostream destination(output_path, error_code, llvm::sys::fs::OF_None);
    if (error_code) {
        *out_error = "No se pudo abrir el archivo objeto '" + output_path + "': " + error_code.message();
        return false;
    }

    llvm::legacy::PassManager pass_manager;
    const bool cannot_emit = target_machine->addPassesToEmitFile(
        pass_manager,
        destination,
        nullptr,
        llvm::CodeGenFileType::ObjectFile);

    if (cannot_emit) {
        *out_error = "El backend LLVM no puede emitir archivo objeto para ese target.";
        return false;
    }

    pass_manager.run(*module_);
    destination.flush();
    return true;
}

bool LlvmEmitter::EmitRuntimeBridgeProgram(const CompileOptions& options) {
    if (options.source_text.empty()) {
        error_ = "No hay codigo fuente para runtime bridge LLVM.";
        return false;
    }

    if (!CreateMainFunction()) {
        return false;
    }

    std::vector<llvm::Type*> runtime_params;
    runtime_params.push_back(llvm::PointerType::getUnqual(builder_.getInt8Ty()));
    runtime_params.push_back(llvm::PointerType::getUnqual(builder_.getInt8Ty()));

    llvm::FunctionType* runtime_type = llvm::FunctionType::get(
        builder_.getInt32Ty(),
        runtime_params,
        false);

    llvm::FunctionCallee runtime_entry =
        module_->getOrInsertFunction("clot_runtime_execute_source", runtime_type);

    llvm::Value* source_literal = builder_.CreateGlobalStringPtr(options.source_text);
    llvm::Value* source_path_literal = builder_.CreateGlobalStringPtr(options.input_path);
    llvm::Value* status = builder_.CreateCall(runtime_entry, {source_literal, source_path_literal});
    builder_.CreateRet(status);

    if (llvm::verifyFunction(*main_function_, &llvm::errs())) {
        error_ = "LLVM genero una funcion main invalida en runtime bridge.";
        return false;
    }

    if (llvm::verifyModule(*module_, &llvm::errs())) {
        error_ = "LLVM genero un modulo invalido en runtime bridge.";
        return false;
    }

    return true;
}

bool LlvmEmitter::CreateMainFunction() {
    llvm::FunctionType* main_type = llvm::FunctionType::get(builder_.getInt32Ty(), false);
    main_function_ = llvm::Function::Create(
        main_type,
        llvm::Function::ExternalLinkage,
        "main",
        module_.get());

    if (main_function_ == nullptr) {
        error_ = "No se pudo crear la funcion main.";
        return false;
    }

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context_, "entry", main_function_);
    builder_.SetInsertPoint(entry);
    current_function_ = main_function_;
    variables_.clear();
    return EnsurePrintfFunction();
}

bool LlvmEmitter::EnsurePrintfFunction() {
    if (printf_function_.getCallee() != nullptr) {
        return true;
    }

    std::vector<llvm::Type*> printf_params;
    printf_params.push_back(llvm::PointerType::getUnqual(builder_.getInt8Ty()));
    llvm::FunctionType* printf_type = llvm::FunctionType::get(
        builder_.getInt32Ty(),
        printf_params,
        true);

    printf_function_ = module_->getOrInsertFunction("printf", printf_type);
    return true;
}

bool LlvmEmitter::DeclareUserFunctions(const frontend::Program& program) {
    user_functions_.clear();
    user_function_order_.clear();

    for (const auto& statement : program.statements) {
        const auto* function_decl = dynamic_cast<const frontend::FunctionDeclStmt*>(statement.get());
        if (function_decl == nullptr) {
            continue;
        }

        if (user_functions_.find(function_decl->name) != user_functions_.end()) {
            error_ = "Funcion duplicada no soportada en AOT LLVM: " + function_decl->name;
            return false;
        }

        std::vector<llvm::Type*> params;
        std::vector<bool> by_reference;
        params.reserve(function_decl->params.size());
        by_reference.reserve(function_decl->params.size());

        for (const auto& param : function_decl->params) {
            if (param.by_reference) {
                params.push_back(llvm::PointerType::getUnqual(builder_.getDoubleTy()));
            } else {
                params.push_back(builder_.getDoubleTy());
            }
            by_reference.push_back(param.by_reference);
        }

        llvm::FunctionType* function_type = llvm::FunctionType::get(builder_.getVoidTy(), params, false);
        llvm::Function* llvm_function = llvm::Function::Create(
            function_type,
            llvm::Function::ExternalLinkage,
            MangleFunctionName(function_decl->name),
            module_.get());

        if (llvm_function == nullptr) {
            error_ = "No se pudo crear la funcion LLVM para '" + function_decl->name + "'.";
            return false;
        }

        UserFunctionInfo info;
        info.declaration = function_decl;
        info.llvm_function = llvm_function;
        info.param_by_reference = std::move(by_reference);

        user_functions_[function_decl->name] = std::move(info);
        user_function_order_.push_back(function_decl->name);
    }

    return true;
}

bool LlvmEmitter::EmitUserFunctions() {
    for (const std::string& function_name : user_function_order_) {
        const auto found = user_functions_.find(function_name);
        if (found == user_functions_.end()) {
            error_ = "Funcion interna no encontrada durante emision LLVM: " + function_name;
            return false;
        }

        if (!EmitUserFunction(found->second)) {
            return false;
        }
    }

    return true;
}

bool LlvmEmitter::EmitUserFunction(const UserFunctionInfo& function_info) {
    if (function_info.declaration == nullptr || function_info.llvm_function == nullptr) {
        error_ = "Funcion LLVM invalida durante emision.";
        return false;
    }

    std::unordered_map<std::string, llvm::Value*> saved_variables = std::move(variables_);
    llvm::Function* saved_function = current_function_;

    current_function_ = function_info.llvm_function;
    variables_.clear();

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(
        context_,
        "entry",
        function_info.llvm_function);
    builder_.SetInsertPoint(entry);

    if (!EnsurePrintfFunction()) {
        variables_ = std::move(saved_variables);
        current_function_ = saved_function;
        return false;
    }

    auto arg_it = function_info.llvm_function->arg_begin();
    for (std::size_t i = 0; i < function_info.declaration->params.size(); ++i, ++arg_it) {
        const frontend::FunctionParam& param = function_info.declaration->params[i];
        llvm::Argument& argument = *arg_it;
        argument.setName(param.name);

        if (param.by_reference) {
            variables_[param.name] = &argument;
            continue;
        }

        llvm::AllocaInst* slot = CreateEntryBlockAlloca(function_info.llvm_function, param.name);
        builder_.CreateStore(&argument, slot);
        variables_[param.name] = slot;
    }

    for (const auto& nested_statement : function_info.declaration->body) {
        if (nested_statement == nullptr || !EmitStatement(*nested_statement, false)) {
            variables_ = std::move(saved_variables);
            current_function_ = saved_function;
            return false;
        }
    }

    if (builder_.GetInsertBlock()->getTerminator() == nullptr) {
        builder_.CreateRetVoid();
    }

    if (llvm::verifyFunction(*function_info.llvm_function, &llvm::errs())) {
        error_ = "LLVM genero una funcion invalida para '" + function_info.declaration->name + "'.";
        variables_ = std::move(saved_variables);
        current_function_ = saved_function;
        return false;
    }

    variables_ = std::move(saved_variables);
    current_function_ = saved_function;
    return true;
}

bool LlvmEmitter::EmitStatement(const frontend::Statement& statement, bool allow_function_declaration) {
    if (const auto* assignment = dynamic_cast<const frontend::AssignmentStmt*>(&statement)) {
        return EmitAssignment(*assignment);
    }

    if (const auto* print = dynamic_cast<const frontend::PrintStmt*>(&statement)) {
        return EmitPrint(*print);
    }

    if (const auto* conditional = dynamic_cast<const frontend::IfStmt*>(&statement)) {
        return EmitIf(*conditional);
    }

    if (const auto* import_stmt = dynamic_cast<const frontend::ImportStmt*>(&statement)) {
        if (import_stmt->module_name == "math") {
            math_module_imported_ = true;
            return true;
        }
        error_ = "Import no soportado en LLVM: " + import_stmt->module_name;
        return false;
    }

    if (dynamic_cast<const frontend::FunctionDeclStmt*>(&statement) != nullptr) {
        if (!allow_function_declaration) {
            error_ = "No se soportan funciones anidadas en modo compile LLVM AOT.";
            return false;
        }
        return true;
    }

    if (const auto* expression_stmt = dynamic_cast<const frontend::ExpressionStmt*>(&statement)) {
        if (expression_stmt->expr == nullptr) {
            error_ = "Sentencia de expresion vacia en emision LLVM.";
            return false;
        }

        if (const auto* call = dynamic_cast<const frontend::CallExpr*>(expression_stmt->expr.get())) {
            return EmitCallStatement(*call);
        }

        llvm::Value* emitted = EmitNumericExpr(*expression_stmt->expr);
        return emitted != nullptr;
    }

    error_ = "Sentencia no soportada por el compilador LLVM.";
    return false;
}

bool LlvmEmitter::EmitAssignment(const frontend::AssignmentStmt& statement) {
    llvm::Value* expression_value = EmitNumericExpr(*statement.expr);
    if (expression_value == nullptr) {
        return false;
    }

    llvm::Value* value_to_store = expression_value;

    if (statement.op == frontend::AssignmentOp::AddAssign ||
        statement.op == frontend::AssignmentOp::SubAssign) {
        auto existing = variables_.find(statement.name);
        if (existing == variables_.end()) {
            error_ = "Variable no definida para asignacion compuesta: " + statement.name;
            return false;
        }

        llvm::Value* current = builder_.CreateLoad(builder_.getDoubleTy(), existing->second, statement.name + ".load");
        if (statement.op == frontend::AssignmentOp::AddAssign) {
            value_to_store = builder_.CreateFAdd(current, expression_value, statement.name + ".add");
        } else {
            value_to_store = builder_.CreateFSub(current, expression_value, statement.name + ".sub");
        }
    }

    llvm::Value* target = nullptr;
    auto found = variables_.find(statement.name);
    if (found == variables_.end()) {
        if (current_function_ == nullptr) {
            error_ = "Estado interno invalido: no hay funcion activa para asignacion.";
            return false;
        }

        target = CreateEntryBlockAlloca(current_function_, statement.name);
        variables_[statement.name] = target;
    } else {
        target = found->second;
    }

    if (statement.declaration_type == frontend::DeclarationType::Byte) {
        llvm::Value* zero = llvm::ConstantFP::get(builder_.getDoubleTy(), 0.0);
        llvm::Value* upper = llvm::ConstantFP::get(builder_.getDoubleTy(), 255.0);
        llvm::Value* clamped_low = builder_.CreateSelect(
            builder_.CreateFCmpOLT(value_to_store, zero),
            zero,
            value_to_store);
        value_to_store = builder_.CreateSelect(
            builder_.CreateFCmpOGT(clamped_low, upper),
            upper,
            clamped_low);
    }

    builder_.CreateStore(value_to_store, target);
    return true;
}

bool LlvmEmitter::EmitPrint(const frontend::PrintStmt& statement) {
    if (!EnsurePrintfFunction()) {
        return false;
    }

    if (const auto* literal = dynamic_cast<const frontend::StringExpr*>(statement.expr.get())) {
        llvm::Value* format = builder_.CreateGlobalStringPtr("%s\n");
        llvm::Value* text = builder_.CreateGlobalStringPtr(literal->value);
        builder_.CreateCall(printf_function_, {format, text});
        return true;
    }

    llvm::Value* numeric_value = EmitNumericExpr(*statement.expr);
    if (numeric_value == nullptr) {
        return false;
    }

    llvm::Value* format = builder_.CreateGlobalStringPtr("%.15g\n");
    builder_.CreateCall(printf_function_, {format, numeric_value});
    return true;
}

bool LlvmEmitter::EmitIf(const frontend::IfStmt& statement) {
    llvm::Value* condition_value = EmitNumericExpr(*statement.condition);
    if (condition_value == nullptr) {
        return false;
    }

    llvm::Value* zero = llvm::ConstantFP::get(builder_.getDoubleTy(), 0.0);
    llvm::Value* condition = builder_.CreateFCmpONE(condition_value, zero, "if.cond");

    llvm::Function* function = builder_.GetInsertBlock()->getParent();
    llvm::BasicBlock* then_block = llvm::BasicBlock::Create(context_, "if.then", function);
    llvm::BasicBlock* else_block = llvm::BasicBlock::Create(context_, "if.else", function);
    llvm::BasicBlock* merge_block = llvm::BasicBlock::Create(context_, "if.end", function);

    builder_.CreateCondBr(condition, then_block, else_block);

    builder_.SetInsertPoint(then_block);
    for (const auto& nested : statement.then_branch) {
        if (nested == nullptr || !EmitStatement(*nested, false)) {
            return false;
        }
    }
    if (builder_.GetInsertBlock()->getTerminator() == nullptr) {
        builder_.CreateBr(merge_block);
    }

    builder_.SetInsertPoint(else_block);
    for (const auto& nested : statement.else_branch) {
        if (nested == nullptr || !EmitStatement(*nested, false)) {
            return false;
        }
    }
    if (builder_.GetInsertBlock()->getTerminator() == nullptr) {
        builder_.CreateBr(merge_block);
    }

    builder_.SetInsertPoint(merge_block);
    return true;
}

bool LlvmEmitter::EmitCallStatement(const frontend::CallExpr& call) {
    if (call.callee == "sum" && math_module_imported_) {
        return EmitBuiltinSumCall(call) != nullptr;
    }

    const auto function_it = user_functions_.find(call.callee);
    if (function_it != user_functions_.end()) {
        return EmitUserFunctionCall(call, false, nullptr);
    }

    if (call.callee == "sum") {
        error_ = "sum(a, b) requiere 'import math;' en modo compile LLVM AOT.";
    } else {
        error_ = "Funcion no soportada en modo compile LLVM AOT: " + call.callee;
    }
    return false;
}

llvm::AllocaInst* LlvmEmitter::CreateEntryBlockAlloca(llvm::Function* function, const std::string& name) {
    llvm::IRBuilder<> entry_builder(&function->getEntryBlock(), function->getEntryBlock().begin());
    return entry_builder.CreateAlloca(builder_.getDoubleTy(), nullptr, name);
}

llvm::Value* LlvmEmitter::EmitBuiltinSumCall(const frontend::CallExpr& call) {
    if (!math_module_imported_) {
        error_ = "sum(a, b) requiere 'import math;' en modo compile LLVM AOT.";
        return nullptr;
    }

    if (call.arguments.size() != 2) {
        error_ = "sum(a, b) requiere exactamente 2 argumentos.";
        return nullptr;
    }

    for (const auto& argument : call.arguments) {
        if (argument.by_reference || argument.value == nullptr) {
            error_ = "sum(a, b) no acepta argumentos por referencia.";
            return nullptr;
        }
    }

    llvm::Value* lhs = EmitNumericExpr(*call.arguments[0].value);
    llvm::Value* rhs = EmitNumericExpr(*call.arguments[1].value);
    if (lhs == nullptr || rhs == nullptr) {
        return nullptr;
    }

    return builder_.CreateFAdd(lhs, rhs, "sum.call");
}

bool LlvmEmitter::EmitUserFunctionCall(
    const frontend::CallExpr& call,
    bool require_numeric_result,
    llvm::Value** out_value) {
    const auto function_it = user_functions_.find(call.callee);
    if (function_it == user_functions_.end()) {
        error_ = "Funcion no definida en modo compile LLVM AOT: " + call.callee;
        return false;
    }

    if (require_numeric_result) {
        error_ = "La funcion '" + call.callee + "' no retorna valor utilizable en expresion.";
        return false;
    }

    const UserFunctionInfo& function_info = function_it->second;
    if (call.arguments.size() != function_info.param_by_reference.size()) {
        error_ = "Numero incorrecto de argumentos para funcion '" + call.callee + "'.";
        return false;
    }

    std::vector<llvm::Value*> emitted_arguments;
    emitted_arguments.reserve(call.arguments.size());

    for (std::size_t i = 0; i < call.arguments.size(); ++i) {
        const frontend::CallArgument& argument = call.arguments[i];
        if (argument.value == nullptr) {
            error_ = "Argumento vacio en llamada a funcion '" + call.callee + "'.";
            return false;
        }

        if (function_info.param_by_reference[i]) {
            const auto* variable = dynamic_cast<const frontend::VariableExpr*>(argument.value.get());
            if (variable == nullptr) {
                error_ = "Parametro por referencia en '" + call.callee + "' requiere una variable.";
                return false;
            }
            if (ContainsDot(variable->name)) {
                error_ = "Referencia por propiedad no soportada en AOT LLVM: " + variable->name;
                return false;
            }

            const auto found = variables_.find(variable->name);
            if (found == variables_.end()) {
                error_ = "Variable no definida para referencia: " + variable->name;
                return false;
            }

            emitted_arguments.push_back(found->second);
            continue;
        }

        if (argument.by_reference) {
            error_ = "No se puede pasar '&' a parametro por valor en llamada '" + call.callee + "'.";
            return false;
        }

        llvm::Value* value = EmitNumericExpr(*argument.value);
        if (value == nullptr) {
            return false;
        }
        emitted_arguments.push_back(value);
    }

    builder_.CreateCall(function_info.llvm_function, emitted_arguments);
    if (out_value != nullptr) {
        *out_value = nullptr;
    }
    return true;
}

llvm::Value* LlvmEmitter::EmitNumericExpr(const frontend::Expr& expression) {
    if (const auto* number = dynamic_cast<const frontend::NumberExpr*>(&expression)) {
        return llvm::ConstantFP::get(builder_.getDoubleTy(), number->value);
    }

    if (const auto* boolean = dynamic_cast<const frontend::BoolExpr*>(&expression)) {
        return llvm::ConstantFP::get(builder_.getDoubleTy(), boolean->value ? 1.0 : 0.0);
    }

    if (const auto* variable = dynamic_cast<const frontend::VariableExpr*>(&expression)) {
        if (ContainsDot(variable->name)) {
            error_ = "Acceso por propiedad no soportado en AOT LLVM: " + variable->name;
            return nullptr;
        }

        auto found = variables_.find(variable->name);
        if (found == variables_.end()) {
            error_ = "Variable no definida: " + variable->name;
            return nullptr;
        }
        return builder_.CreateLoad(builder_.getDoubleTy(), found->second, variable->name + ".value");
    }

    if (dynamic_cast<const frontend::StringExpr*>(&expression) != nullptr) {
        error_ = "Las expresiones string solo se soportan como literal directo en print dentro del modo compilado.";
        return nullptr;
    }

    if (dynamic_cast<const frontend::ListExpr*>(&expression) != nullptr) {
        error_ = "Las listas aun no se soportan en modo compile LLVM AOT.";
        return nullptr;
    }

    if (dynamic_cast<const frontend::ObjectExpr*>(&expression) != nullptr) {
        error_ = "Los objetos aun no se soportan en modo compile LLVM AOT.";
        return nullptr;
    }

    if (dynamic_cast<const frontend::IndexExpr*>(&expression) != nullptr) {
        error_ = "La indexacion de listas aun no se soporta en modo compile LLVM AOT.";
        return nullptr;
    }

    if (const auto* call = dynamic_cast<const frontend::CallExpr*>(&expression)) {
        if (call->callee == "sum" && math_module_imported_) {
            return EmitBuiltinSumCall(*call);
        }

        if (user_functions_.find(call->callee) != user_functions_.end()) {
            error_ = "La funcion '" + call->callee + "' no retorna valor utilizable en expresion.";
            return nullptr;
        }

        error_ = "Llamada no soportada en modo compile LLVM AOT: " + call->callee;
        return nullptr;
    }

    if (const auto* unary = dynamic_cast<const frontend::UnaryExpr*>(&expression)) {
        llvm::Value* operand = EmitNumericExpr(*unary->operand);
        if (operand == nullptr) {
            return nullptr;
        }

        switch (unary->op) {
        case frontend::UnaryOp::Plus:
            return operand;
        case frontend::UnaryOp::Negate:
            return builder_.CreateFNeg(operand, "neg");
        case frontend::UnaryOp::LogicalNot: {
            llvm::Value* zero = llvm::ConstantFP::get(builder_.getDoubleTy(), 0.0);
            llvm::Value* bool_value = builder_.CreateFCmpUEQ(operand, zero, "not.bool");
            return builder_.CreateUIToFP(bool_value, builder_.getDoubleTy(), "not.num");
        }
        }
    }

    if (const auto* binary = dynamic_cast<const frontend::BinaryExpr*>(&expression)) {
        llvm::Value* lhs = EmitNumericExpr(*binary->lhs);
        llvm::Value* rhs = EmitNumericExpr(*binary->rhs);
        if (lhs == nullptr || rhs == nullptr) {
            return nullptr;
        }

        switch (binary->op) {
        case frontend::BinaryOp::Add:
            return builder_.CreateFAdd(lhs, rhs, "add");
        case frontend::BinaryOp::Subtract:
            return builder_.CreateFSub(lhs, rhs, "sub");
        case frontend::BinaryOp::Multiply:
            return builder_.CreateFMul(lhs, rhs, "mul");
        case frontend::BinaryOp::Divide:
            return builder_.CreateFDiv(lhs, rhs, "div");
        case frontend::BinaryOp::Modulo:
            return builder_.CreateFRem(lhs, rhs, "mod");
        case frontend::BinaryOp::Power: {
            llvm::Function* pow_fn = llvm::Intrinsic::getDeclaration(
                module_.get(),
                llvm::Intrinsic::pow,
                {builder_.getDoubleTy()});
            return builder_.CreateCall(pow_fn, {lhs, rhs}, "pow");
        }
        case frontend::BinaryOp::Equal:
            return BoolToNumber(builder_.CreateFCmpUEQ(lhs, rhs, "eq"));
        case frontend::BinaryOp::NotEqual:
            return BoolToNumber(builder_.CreateFCmpUNE(lhs, rhs, "neq"));
        case frontend::BinaryOp::Less:
            return BoolToNumber(builder_.CreateFCmpOLT(lhs, rhs, "lt"));
        case frontend::BinaryOp::LessEqual:
            return BoolToNumber(builder_.CreateFCmpOLE(lhs, rhs, "lte"));
        case frontend::BinaryOp::Greater:
            return BoolToNumber(builder_.CreateFCmpOGT(lhs, rhs, "gt"));
        case frontend::BinaryOp::GreaterEqual:
            return BoolToNumber(builder_.CreateFCmpOGE(lhs, rhs, "gte"));
        case frontend::BinaryOp::LogicalAnd: {
            llvm::Value* lhs_bool = builder_.CreateFCmpONE(lhs, llvm::ConstantFP::get(builder_.getDoubleTy(), 0.0));
            llvm::Value* rhs_bool = builder_.CreateFCmpONE(rhs, llvm::ConstantFP::get(builder_.getDoubleTy(), 0.0));
            return BoolToNumber(builder_.CreateAnd(lhs_bool, rhs_bool, "and"));
        }
        case frontend::BinaryOp::LogicalOr: {
            llvm::Value* lhs_bool = builder_.CreateFCmpONE(lhs, llvm::ConstantFP::get(builder_.getDoubleTy(), 0.0));
            llvm::Value* rhs_bool = builder_.CreateFCmpONE(rhs, llvm::ConstantFP::get(builder_.getDoubleTy(), 0.0));
            return BoolToNumber(builder_.CreateOr(lhs_bool, rhs_bool, "or"));
        }
        }
    }

    error_ = "Expresion no soportada en backend LLVM.";
    return nullptr;
}

llvm::Value* LlvmEmitter::BoolToNumber(llvm::Value* bool_value) {
    return builder_.CreateUIToFP(bool_value, builder_.getDoubleTy(), "bool.num");
}

std::string LlvmEmitter::MangleFunctionName(const std::string& name) const {
    return "clot_fn_" + name;
}

}  // namespace clot::codegen::internal

#endif  // CLOT_HAS_LLVM
