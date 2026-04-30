# Changelog

All notable changes to the "clotlang-syntax" extension will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.0.1] - 2026-04-27

### Added
- **Initial Release** of the official ClotLang syntax highlighting extension.
- Comprehensive TextMate grammar (`clot.tmLanguage.json`) supporting the ClotLang specification.
- Complete syntax highlighting for control flow keywords (`if`, `while`, `for`, `for-each`, `switch`, `try/catch/finally`, `defer`).
- Full support for OOP MVP constructs, including classes, interfaces, inheritance, and access modifiers (`public`, `private`, `protected`, `readonly`, `override`).
- Accurate coloring for the type system, including primitives and generic collections (e.g., `list<int>`, `map<string, User>`).
- Built-in recognition for the ClotLang standard library, I/O functions, and math constants.
- Accurate parsing for string interpolations and escape characters.
- Official extension icon and publisher metadata.

## [0.0.2] - 2026-04-28
- New file icon