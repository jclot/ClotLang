# ClotLang Syntax Support

Official Visual Studio Code extension providing comprehensive syntax highlighting for the **Clot** programming language. 

This extension ensures that `.clot` files are perfectly formatted, whether you are writing quick scripts for the interpreter or structuring complex OOP models for native LLVM AOT compilation.

## Features

This extension utilizes TextMate grammars to provide accurate, industry-standard syntax highlighting. 

* **Complete Keyword Coverage:** Full support for ClotLang's control flow (`if`, `while`, `for`, `switch`, `defer`), error handling (`try`, `catch`, `finally`), and logical operators.
* **OOP MVP Support:** Highlights classes, interfaces, access modifiers (`public`, `private`, `protected`), and object-oriented keywords (`extends`, `implements`, `override`, `abstract`).
* **Type System Recognition:** Accurate coloring for built-in types (`int`, `double`, `string`, `list`, `map`, etc.) including gradual typing and generics syntax (e.g., `list<int>`, `map<string, User>`).
* **Built-in Functions:** Native highlighting for the standard library, including I/O (`println`, `printf`), math module functions, system builtins (`async_read_file`), and type helpers (`cast`, `isinstance`).
* **String Interpolation:** Correct parsing of escape characters and interpolated variables within strings (e.g., `"Hello {name}"`).

> **Tip:** Pair this extension with your favorite VS Code theme. The grammar is mapped to standard scopes, ensuring ClotLang looks great in both dark and light modes.

## Requirements

No special requirements or external dependencies are needed. Simply install the extension and open any `.clot` file.

To execute or compile your code, ensure you have the Clot CLI installed on your system.
* [ClotLang Installation Guide & Source](https://github.com/jclot/ClotLang)
* [Official Documentation](https://jclot.github.io/ClotLang/docs-page.html)

## Extension Settings

This extension currently focuses purely on syntax highlighting and does not contribute any custom workspace settings. 

*Language server features (like autocomplete, jump-to-definition, and inline diagnostics) are planned for future releases.*

## Known Issues

* Complex nested string interpolations might occasionally lose scope tracking depending on the active VS Code theme.
* Syntax highlighting for multi-line enums works best when the block is properly terminated with a semicolon.

If you find a syntax highlighting bug or a missing token, please report it on the [ClotLang Issue Tracker](https://github.com/jclot/ClotLang/issues).

## Release Notes

### 1.0.0

* Initial release.
* Added full TextMate grammar (`clot.tmLanguage.json`) for the `.clot` extension.
* Support for ClotLang's core features as of the March 2026 specification (OOP MVP, gradual typing, standard library builtins).

---

**Enjoy coding in Clot!**