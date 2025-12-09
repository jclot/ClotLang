# The Clot Programming Language

Clot is an imperative, whitespace-aware programming language built for quick experiments in mathematics, physics, and artificial intelligence. The interpreter is implemented in modern **C++20**, prioritizes Python-like readability, and executes source files line by line for fast feedback.

> **At a glance (ES)**: Clot es un lenguaje imperativo diseñado para prototipado rápido en matemáticas, física e IA. El intérprete en C++20 ejecuta archivos línea por línea, con sintaxis cercana a Python y reglas estrictas de tabulación en funciones y bloques condicionales.

## Table of Contents
- [Project Status](#project-status)
- [Features](#features)
- [Quick Start](#quick-start)
- [Language Guide](#language-guide)
  - [File Structure and Statements](#file-structure-and-statements)
  - [Data Types](#data-types)
  - [Expressions and Operators](#expressions-and-operators)
  - [Assignments and Mutation](#assignments-and-mutation)
  - [Standard Output: `print`](#standard-output-print)
  - [Functions](#functions)
  - [Conditionals](#conditionals)
  - [Modules](#modules)
  - [Comments](#comments)
- [Example Program](#example-program)
- [Repository Layout](#repository-layout)
- [Development Notes](#development-notes)
- [License](#license)

## Project Status
- Executes source files sequentially, emitting diagnostic traces for each assignment and branch to aid debugging.
- Supports numbers, booleans, strings, lists, objects, functions (with pass-by-reference), nested conditionals, and basic module import.
- Loop constructs are **not yet implemented**; roadmap items include iteration, richer modules, and additional numeric types.

## Features
- **C++20 interpreter** with minimal external dependencies and portable Makefile/Visual Studio projects for Unix-like and Windows environments.【F:ClotProgrammingLanguage/ClotProgrammingLanguage.cpp†L1-L26】【F:Makefile†L3-L31】
- **Immediate execution**: tokens are parsed and evaluated line by line, making it easy to trace program state.【F:ClotProgrammingLanguage/Tokenizer.cpp†L15-L58】【F:ClotProgrammingLanguage/ExpressionEvaluator.cpp†L18-L73】
- **Strong debugging signals**: variable assignments print informative messages, including type validation for `long` and `byte`.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L18-L145】
- **Pass-by-reference semantics** in function parameters enable in-place updates from callee to caller.【F:ClotProgrammingLanguage/FunctionExecution.cpp†L28-L116】
- **Extensible module importer** starting with a `math` helper that exposes `sum(a, b)`.【F:ClotProgrammingLanguage/ModuleImporter.cpp†L5-L35】

## Quick Start
1. **Prerequisites**: a C++20-capable compiler (e.g., `g++`).
2. **Build the interpreter**:
   ```bash
   make
   ```
   This compiles the sources and produces `ClotProgrammingLanguage/clot.exe`. The target names match Visual Studio defaults while remaining compatible with Unix toolchains.【F:Makefile†L3-L31】
3. **Run the bundled sample**:
   ```bash
   make run
   ```
   Executes `ClotProgrammingLanguage/test.clot` with the freshly built interpreter. Use `make build-run` to compile and run in one step.【F:Makefile†L17-L25】
4. **Execute a custom program**: pass your `.clot` file to the interpreter binary, for example `./ClotProgrammingLanguage/clot.exe path/to/program.clot`.

## Language Guide
### File Structure and Statements
- Each instruction ends with `;`, including `print` and function calls.【F:ClotProgrammingLanguage/Tokenizer.cpp†L15-L58】
- Empty lines are ignored. Indentation with tabs is significant inside function bodies and conditional blocks.
- Execution proceeds from top to bottom; imported modules extend the available functions.

### Data Types
- **Floating-point (`double`)**: default numeric representation.
- **Integers (`int`)**: inferred when a numeric literal lacks a decimal point.
- **Long integers (`long`)**: declared with `long name = value;` and validated against `long long` limits.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L18-L47】
- **Bytes (`byte`)**: values in the `[0, 255]` range via `byte name = value;`.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L18-L47】
- **Strings (`string`)**: double-quoted literals with `+` concatenation and implicit numeric-to-text conversion.【F:ClotProgrammingLanguage/ExpressionEvaluator.cpp†L105-L154】
- **Booleans**: `true` or `false`, also produced by comparisons and logical operators; booleans coerce to `1.0` or `0.0` when used numerically.【F:ClotProgrammingLanguage/ExpressionEvaluator.cpp†L18-L73】
- **Lists**: `[value1, value2, ...]` containing numbers, strings, or booleans.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L51-L79】
- **Objects**: `{key: value, ...}` holding numbers, strings, booleans, or lists.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L80-L109】

### Expressions and Operators
Operators are evaluated with the following precedence (high to low):
1. Exponentiation `^`
2. Multiplication `*`, division `/`, modulus `%`
3. Addition/subtraction `+`, `-`
4. Comparisons `==`, `!=`, `<`, `<=`, `>`, `>=`
5. Logical `&&`, `||`, `!`

Expressions combine literals, variables, and operators. Boolean outcomes are represented internally as `1.0` (true) or `0.0` (false).【F:ClotProgrammingLanguage/ExpressionEvaluator.cpp†L18-L103】

### Assignments and Mutation
- **Simple assignment**: `x = 1 + 2;`
- **Compound assignment**: `x += 3;` or `x -= 2;` for pre-declared numeric variables.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L124-L145】
- **Typed declarations**: `long` and `byte` require explicit type keywords and enforce range checks.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L18-L47】
- **Collections**: lists and objects are declared inline with `[]` or `{}` initializers.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L51-L109】
- Each assignment logs its computed value and resolved type to the console, which simplifies tracing program state.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L18-L145】

### Standard Output: `print`
Use `print(expression);` to display values:
- Accepts strings, booleans, numbers, lists, objects, variables, and inline expressions.
- Auto-detects whether to evaluate numerically or concatenate as text for mixed expressions.【F:ClotProgrammingLanguage/PrintStatement.cpp†L9-L138】
- Collections render with delimiters: lists in `[ ]` and objects in `{ }` with their key/value pairs.【F:ClotProgrammingLanguage/PrintStatement.cpp†L37-L87】

### Functions
**Declaration**
```
func name(param1, &param2):
        // tab-indented body
endfunc
```
- Parameters prefixed with `&` are passed **by reference** and propagate modifications to the caller’s variable.【F:ClotProgrammingLanguage/FunctionExecution.cpp†L28-L116】
- Bodies are captured until `endfunc`; each line inside must start with a tab to belong to the function.【F:ClotProgrammingLanguage/FunctionDeclaration.cpp†L33-L57】

**Invocation**
```
name(arg1, arg2);
```
- Arity must match the declaration.
- Reference parameters require an existing identifier at the call site; value parameters are evaluated and copied.
- Local numeric variables are restored when the function exits, except those passed by reference, which reflect in-place changes.【F:ClotProgrammingLanguage/FunctionExecution.cpp†L85-L139】

### Conditionals
```
if condition:
        // if block (tab-indented)
else:
        // else block (optional)
endif
```
- Conditions evaluate using the numeric/boolean rules; any non-zero result is treated as truthy.【F:ClotProgrammingLanguage/ConditionalStatement.cpp†L1-L78】
- Supports nested `if`/`else` blocks, both at top level and inside functions.
- Branch bodies are stored and then executed line by line, allowing assignments, prints, imports, function calls, or further conditionals.【F:ClotProgrammingLanguage/ConditionalStatement.cpp†L80-L215】

### Modules
Import helper modules with `import name;`:
- **math**: exposes `sum(a, b)`, returning the numeric sum of its two arguments.【F:ClotProgrammingLanguage/ModuleImporter.cpp†L5-L35】
- Imports are validated; unknown modules or missing semicolons trigger runtime errors.【F:ClotProgrammingLanguage/ModuleImporter.cpp†L5-L21】

### Comments
Single-line comments start with `//` and are ignored by the interpreter.【F:ClotProgrammingLanguage/Tokenizer.cpp†L15-L26】

## Example Program
The following sample demonstrates types, conditionals, references, and module usage:
```clot
// Variables and types
long max = 9000;
byte level = 42;
flag = true;
nums = [1, 2, 3];
user = {name: "Ada", active: true, scores: nums};

// Function with reference
func inc(&value):
        value += 1;
endfunc

// Conditional
if(flag && max > 100):
        print("User: " + user.name + " level " + level);
        inc(max);
else:
        print("Disabled");
endif

// Module usage
import math;
print(sum(5, 7));
```

## Repository Layout
- `ClotProgrammingLanguage/`: C++ source code for the interpreter and produced binary `clot.exe`.
- `ClotProgrammingLanguage/test.clot`: default sample script executed by `main()`.
- `TokenizerTests/`: placeholder for tokenizer unit tests.
- `Makefile`: build and run targets for `g++` toolchains.
- `ClotProgrammingLanguage/ClotProgrammingLanguage.vcxproj*`: Visual Studio project configuration for Windows developers.
- `LICENSE.txt`: MIT license.

## Development Notes
- The interpreter processes input line by line, so malformed statements surface immediately with clear diagnostics.
- When adding features, keep indentation rules consistent: tab-indented blocks are significant inside functions and conditionals.
- Module registration is centralized in `ModuleImporter.cpp`; new modules can be added by extending its validation and dispatch tables.

## License
This project is distributed under the MIT License. See [`LICENSE.txt`](LICENSE.txt) for details.
