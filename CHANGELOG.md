# Changelog

All notable changes to the Clot Programming Language are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

[0.3.0]: https://github.com/jclot/ClotLang/releases/tag/v0.3.0
[0.2.0]: https://github.com/jclot/ClotLang/releases/tag/v0.2.0
