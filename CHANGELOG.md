# Changelog

All notable changes to the Clot Programming Language are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.3.3] - 2026-07-07

### Fixed

- **A plain `import mod;` now exposes the module as a dot-access namespace,**
  symmetric with `import mod as alias;`. Previously only the aliased form created
  a namespace object, so `import inventario;` followed by `inventario.Inventory()`
  failed with `Undefined variable: inventario`, even though `import inventario as
  inv;` made `inv.Inventory()` work. Now both forms bind a namespace object — the
  handle is the module's last dotted segment (`scripts.math` → `math`,
  `inventario` → `inventario`) or the alias when given. The module's public
  symbols remain available unqualified in both cases, so existing code keeps
  working.

## [0.3.2] - 2026-07-07

### Changed

- **String interpolation now uses f-strings.** Interpolation happens only inside
  strings prefixed with `f` (`f"Hello {name}"`), matching Python 3 / C#. Plain
  `"..."` strings are fully literal, so braces and `$` carry no special meaning
  and need no escaping. Inside an f-string, `{{` and `}}` still emit literal
  braces.
- **`range()` is now lazy.** It returns a `range` object (as in Python 3, Rust,
  and C#) that yields each value on demand instead of building a list up front.
  A `for` loop over a range uses O(1) memory, and the previous 1,000,000-element
  cap is gone, so arbitrarily large ranges are allowed. Ranges also support
  `len`, indexing, and the `in` operator in O(1), and print as `range(start,
  stop[, step])`.

### Removed

- The old brace interpolation for plain strings (`"Hello {name}"` with `{{` /
  `}}` for literal braces). Use `f"Hello {name}"` instead.
- The 1,000,000-element limit on `range()`.

## [0.3.1] - 2026-07-05

### Fixed

- **The bundled standard library now resolves after a normal install.**
  Previously, imports such as `import clot.core.exceptions;` only worked when a
  `clot/` folder happened to sit in a parent directory of the script, so a
  freshly installed `clot` could not find the exceptions library (or any other
  bundled module). The interpreter and analyzer now also look for the standard
  library relative to the `clot` binary — at `<prefix>/lib/clot` — and honor a
  `CLOT_HOME` environment variable, independent of the current working
  directory and of the CLI language.

### Changed

- Release archives now ship the standard library alongside the binary (under
  `lib/clot`), and both installers (`install.sh` / `install.ps1`) place it at
  `<prefix>/lib/clot`. The `cmake --install` step installs it there too.

## [0.3.0] - 2026-07-02

### Added

- **Abstract classes and methods.** New `abstract` keyword for both classes
  (`abstract class ...`) and methods. Abstract methods are declared with an
  empty body and must be overridden by concrete subclasses.
- **`protected` visibility.** Formalized as a first-class member visibility
  modifier alongside `public` and `private`.
- **User-defined types in annotations.** Type annotations now accept custom
  class names (e.g. `func Dual f(x: Dual):`), in addition to the built-in
  types. Internally `DeclarationType` gained `String`, `Bool`, `Null`, and
  `Custom`, and annotations carry a `custom_name`.
- **Standard library: automatic differentiation** (`clot.science.calculus.dual`).
  - `Dual` class (dual numbers) with `add`, `sub`, `mul`, `div`, `powf`,
    `exp`, `log`, `sin`, `cos`, `tan`, `atan`, and `sqrt`.
  - `to_dual(...)`, a `Derivative` class, and the `differentiate(f)` and
    `differentiate_at(f, x)` helper functions.
- **Dedicated `.` (dot) token** in the tokenizer, improving decimal/float
  literal handling (`1.0`) and member access parsing.
- **Multiline `return`** support and additional collection ergonomics.
- **`clot --version` / `clot -v`** print the CLI version (sourced from the
  CMake project version).

### Changed

- **English is now the default CLI language.** Help text and diagnostics print
  in English out of the box; pass `--lang es` or set `CLOT_LANG=es` for Spanish.
- **Expression parser** reworked for more robust member access and call
  chaining (`factory().build().run()`).
- **Call dispatch** now supports an argument offset, fixing `this` binding and
  chained method calls.
- **Module import resolution** in the CLI: dotted module paths
  (`module.submodule`) resolve to filesystem paths with clearer error
  reporting.
- **Internationalization**: translated the new diagnostics (including all the
  abstract-class validations) and added the missing ES→EN translation for a
  `return` statement error.

### Validations (runtime / static analyzer)

- A concrete class cannot declare `abstract` methods.
- An `abstract` method cannot be `private` or `static`, and must have an empty
  body.
- A concrete subclass must implement every inherited `abstract` method.
- An `abstract` class cannot be instantiated.

### Fixed

- Installer script URLs updated from the `main` branch to `master`.

## [0.2.0] - 2026-03-12

- First packaged release with unified `clot` CLI, modular interpreter, optional
  LLVM AOT backend (`--mode compile`), OOP MVP (`class`/`interface`,
  inheritance, interfaces, `get`/`set`, access modifiers), gradual/optional
  typing with runtime validation, collection builtins, exception handling
  (`try`/`catch`/`finally`), `defer`, string interpolation, and multi-file
  module imports.

[0.3.3]: https://github.com/jclot/ClotLang/releases/tag/v0.3.3
[0.3.2]: https://github.com/jclot/ClotLang/releases/tag/v0.3.2
[0.3.1]: https://github.com/jclot/ClotLang/releases/tag/v0.3.1
[0.3.0]: https://github.com/jclot/ClotLang/releases/tag/v0.3.0
[0.2.0]: https://github.com/jclot/ClotLang/releases/tag/v0.2.0
