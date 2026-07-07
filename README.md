<div align="center">
  <img src="docs/assets/images/iconos/clot-nb.png" alt="Clot Programming Language Logo" width="200" />
  
  <h1>Clot Programming Language</h1>

  [![Build Status](https://img.shields.io/badge/build-passing-brightgreen)](#) 
  [![License](https://img.shields.io/badge/license-MIT-blue)](#) 
  [![Version](https://img.shields.io/badge/version-v0.3.0-lightgrey)](#)
</div>


Clot is a multiparadigm programming language (imperative/procedural, with lightweight object-oriented and functional features) designed to offer both flexibility and performance. It features a **dual execution** model, allowing developers to run code rapidly via its built-in interpreter or compile it natively (AOT) utilizing an LLVM backend.

## Resources

* **Source Code:** https://github.com/jclot/ClotLang
* **Architecture Documentation:** `docs/architecture.md`
* **Issue Tracker:** https://github.com/jclot/ClotLang/issues

---

## Key Features

* **Dual Execution:** Interpreter mode for rapid iteration (`--mode interpret`) and native AOT compilation with LLVM for maximum performance (`--mode compile`).
* **Gradual Typing:** Support for pure dynamic typing or explicit type hinting with runtime validation (`list<int>`, `map<string, int>`).
* **Object-Oriented Programming (MVP):** Support for `class`, `interface`, single inheritance (`extends`), contracts (`implements`), access modifiers (`public`, `private`, `protected`), and polymorphism (`override`).
* **Modern and Ergonomic Syntax:** String interpolation with f-strings (`f"Hello {name}"`), method chaining (`obj.a().b()`), membership operators (`in`), and robust exception handling (`try/catch/finally/defer`).
* **Growing Standard Library:** Built-in utilities such as `len`, `range`, `enumerate`, `zip`, base conversions (`hex`, `bin`), and support for multi-file module imports.

---

## Quick Installation (End Users)

To download and install the latest pre-compiled version of the Clot interpreter:

**Linux / macOS** (installs to `~/.local/bin`):
```bash
curl -fsSL [https://raw.githubusercontent.com/jclot/ClotLang/master/scripts/install.sh](https://raw.githubusercontent.com/jclot/ClotLang/master/scripts/install.sh) | bash
```

**Windows PowerShell** (installs to `%LOCALAPPDATA%\Clot\bin`):
```powershell
iwr -useb [https://raw.githubusercontent.com/jclot/ClotLang/master/scripts/install.ps1](https://raw.githubusercontent.com/jclot/ClotLang/master/scripts/install.ps1) | iex
```

Once installed, you can execute your scripts:
```bash
clot program.clot
```
> **Note:** Pre-compiled binaries only include the interpreter. To utilize the compiler mode (`--mode compile`), you must build the project from source with LLVM installed.

<details>
<summary><strong>Advanced Installation and Uninstallation Options</strong></summary>

**Pin a specific version:**
* Linux/macOS: `CLOT_VERSION=v0.3.0 curl ...`
* Windows: `$env:CLOT_VERSION="v0.3.0"; iwr ...`

**Uninstall:**
* Linux/macOS: `bash scripts/uninstall.sh`
* Windows: `iwr -useb https://raw.githubusercontent.com/jclot/ClotLang/master/scripts/uninstall.ps1 | iex`
</details>

---

## A Quick Look at Clot

Clot combines a clean syntax with powerful control structures and optional typing:

```clot
// Module import
import math;

// Functions with optional typing
func list<int> filter_evens(xs):
    list<int> result = [];
    for (item in xs):
        if (item % 2 == 0):
            result.append(item);
        endif
    endfor
    return result;
endfunc

// Exception handling and deferred execution
try:
    defer println("Cleanup finished.");
    numbers = range(10); // lazy: values are produced on demand
    evens = filter_evens(numbers);
    println(f"Evens: {evens}");
catch(Error err):
    println(f"An error occurred: {err}");
endtry
```

---

## Build Instructions (Developers)

If you wish to contribute to the language or use the LLVM-based AOT compiler, you will need to build Clot from source.

### Prerequisites
* Ubuntu 24.04+ (native or via WSL2).
* `cmake` 3.20+ and build tools (`make` or `ninja`).
* LLVM/Clang (required for `--mode compile`).

**Installing LLVM dependencies on WSL:**
```bash
./scripts/install_llvm_wsl.sh
```

### Building the Project (CMake)

Clot utilizes CMake for its build system. The recommended workflow using presets for WSL is:

```bash
cmake --preset wsl-release
cmake --build --preset build-release -j4
```
*The resulting binary will be generated at `./build/wsl-release/clot`.*

### Recommended Environment: Visual Studio 2026 + WSL
1. Open the repository folder in Visual Studio.
2. Select the WSL (Ubuntu) toolchain.
3. Configure CMake with the following argument: `-DCLOT_ENABLE_LLVM=ON`
4. Build the `clot` target.

---

## Advanced Usage and CLI

The unified `clot` CLI manages both interpretation and compilation workflows.

**Interpreter Mode:**
```bash
clot program.clot
```

**LLVM Compiler Mode (AOT):**
```bash
# Generate a native executable
clot program.clot --mode compile --emit exe -o my_program

# Generate an object file
clot program.clot --mode compile --emit obj -o program.o

# View LLVM Intermediate Representation (IR)
clot program.clot --mode compile --emit ir -o program.ll
```

> **Internationalization:** Clot supports diagnostics in multiple languages. You can force English output by using the `--lang en` flag or setting the `CLOT_LANG=en` environment variable.

---

## Testing and Development

To ensure the stability of your local changes, run the provided test suites:

```bash
# Smoke tests
tests/smoke.sh ./build/wsl-release/clot

# LLVM compilation tests
tests/llvm_smoke.sh ./build/wsl-release/clot

# Full local check and benchmarks
scripts/check.sh
benchmarks/baseline.sh ./build/wsl-release/clot

# Run
./build/wsl-release/clot test.clot --lang es
```

---

## Project Architecture

Clot is designed with a modular architecture, maintaining a strict separation between the frontend, the runtime environment, and the code generation backends:

* **`frontend/`**: Tokenizer, Parser, AST, and source loading mechanisms.
* **`interpreter/`**: Modular interpreter designed for direct execution.
* **`codegen/`**: LLVM backend responsible for AOT compilation.
* **`runtime/`**: Core data structures, I18N, and value management.

For a comprehensive view of feature support (detailing what is natively supported in AOT versus the Interpreter), please refer to the [Architecture Documentation](docs/architecture.md).
