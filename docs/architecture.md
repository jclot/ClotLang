# Clot Architecture

## Layers

- `src/cli`: command-line entrypoint and orchestration.
- `src/frontend`: tokenizer + parser + AST construction.
- `src/interpreter`: runtime execution of AST.
- `src/codegen`: LLVM backend and runtime bridge generation.
- `src/runtime`: shared runtime utilities (`Value`, i18n).

## Frontend Internal Split

- `src/frontend/parser_core.cpp`: bloque principal (`Parse`, `ParseBlock`, despacho de sentencias).
- `src/frontend/parser_statements.cpp`: parseo de sentencias (`assignment`, `if`, `func`, `import`, `mutation`, `return`).
- `src/frontend/parser_expression.cpp`: parser de expresiones por precedencia.
- `src/frontend/parser_support.hpp`: utilidades internas compartidas del parser.

## LLVM Backend Internal Split

- `src/codegen/llvm_compiler.cpp`: facade publica (`LlvmCompiler`) y pipeline alto nivel.
- `src/codegen/llvm_aot_support.cpp`: analisis de compatibilidad AOT.
- `src/codegen/llvm_emitter.cpp`: emision IR/objeto y lowering AST->LLVM.
- `src/codegen/llvm_linker.cpp`: enlazado con `clang++` y armado runtime bridge.
- `src/codegen/llvm_backend_internal.hpp`: contratos internos compartidos del backend.

## Execution Modes

- `--mode interpret`: parse + execute AST directly.
- `--mode compile`: compile to LLVM IR/object/executable.
  - AOT path: numeric/function subset.
  - Runtime bridge path: full language features.

## Interpreter Internal Split

- `src/interpreter/interpreter.cpp`: execution core (statements, expressions, calls).
- `src/interpreter/interpreter_state.cpp`: state/mutation/value-normalization logic.
- `src/interpreter/interpreter_modules.cpp`: module resolution/loading/import graph control.

## Module Resolution

- `import a.b.c;` resolves to `a/b/c.clot`.
- Resolution is relative to the importing file directory.
- Circular imports are detected and rejected.

## Internationalization

- Default language: Spanish (`es`).
- English supported via `--lang en` or `CLOT_LANG=en`.
- i18n pipeline lives in `src/runtime/i18n.cpp`.
