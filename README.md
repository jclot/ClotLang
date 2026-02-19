# Clot Programming Language

Refactor de arquitectura para separar claramente frontend, runtime, interprete y backend LLVM.

## Estado actual
- Nuevo CLI unificado: `clot`.
- Nuevo frontend (tokenizador + parser + AST) en `include/clot/frontend` y `src/frontend`.
- Nuevo interprete modular en `include/clot/interpreter` y `src/interpreter`.
- Backend LLVM para compilacion AOT (`--mode compile`) en `include/clot/codegen` y `src/codegen`.
- Internacionalizacion basica de interfaz y diagnosticos (`--lang es|en`, `CLOT_LANG`).
- Migracion funcional completada en modo interprete: listas, objetos, indexacion, propiedades, funciones y `import math`.

## Estructura
```text
include/
  clot/
    codegen/
      llvm_compiler.hpp
    frontend/
      ast.hpp
      parser.hpp
      source_loader.hpp
      token.hpp
      tokenizer.hpp
    interpreter/
      interpreter.hpp
    runtime/
      i18n.hpp
      value.hpp
src/
  cli/
    main.cpp
  codegen/
    llvm_aot_support.cpp
    llvm_backend_internal.hpp
    llvm_compiler.cpp
    llvm_emitter.cpp
    llvm_linker.cpp
    runtime_bridge.cpp
  frontend/
    parser_core.cpp
    parser_expression.cpp
    parser_statements.cpp
    parser_support.hpp
    source_loader.cpp
    tokenizer.cpp
  interpreter/
    interpreter.cpp
    interpreter_modules.cpp
    interpreter_state.cpp
  runtime/
    i18n.cpp
scripts/
  check.sh
  install_llvm_wsl.sh
CMakeLists.txt
Makefile
```

## Requisitos
- Ubuntu 24.04+ en WSL2.
- `cmake` 3.20+.
- `ninja` recomendado.
- LLVM/Clang para usar `--mode compile`.

## Instalar LLVM en WSL
```bash
./scripts/install_llvm_wsl.sh
```
Nota: el script usa `sudo` y pedira tu password de Ubuntu en WSL.

Instala:
- `clang`
- `lld`
- `llvm`
- `llvm-dev`
- `libclang-dev`

## Compilar
```bash
cmake -S . -B build -G "Unix Makefiles" -DCLOT_ENABLE_LLVM=ON
cmake --build build
```

Con presets (util para Visual Studio):
```bash
cmake --preset wsl-release
cmake --build --preset build-release
```

Alternativa:
```bash
make
```

## Uso
### Modo interprete
```bash
./build/clot programa.clot
```

Modo ingles:
```bash
./build/clot programa.clot --lang en
# o:
CLOT_LANG=en ./build/clot programa.clot
```

### Modo compilador LLVM
Generar ejecutable:
```bash
./build/clot programa.clot --mode compile --emit exe -o programa
```

Generar ejecutable en ingles (diagnosticos de compilacion):
```bash
./build/clot programa.clot --mode compile --emit exe -o programa --lang en
# o:
CLOT_LANG=en ./build/clot programa.clot --mode compile --emit exe -o programa
```

Ejecutar binario compilado en ingles (si usa runtime bridge):
```bash
CLOT_LANG=en ./programa
```

Generar objeto:
```bash
./build/clot programa.clot --mode compile --emit obj -o programa.o
```

Generar IR LLVM:
```bash
./build/clot programa.clot --mode compile --emit ir -o programa.ll
```

## Pruebas rapidas
```bash
tests/smoke.sh ./build/clot
```

Prueba de compilacion LLVM con features completas:
```bash
tests/llvm_smoke.sh ./build/wsl-release/clot
```

Chequeo local completo:
```bash
scripts/check.sh
```

## Alcance actual
Soportado en modo interprete:
- Asignaciones numericas (`=`, `+=`, `-=`)
- Declaraciones `long` y `byte` (tratadas como numericas)
- `print(...)` de expresiones numericas y string
- `if / else / endif`
- Operadores aritmeticos, comparacion y logicos
- Listas y objetos (incluye `nums[2]` y `user.name`)
- Mutaciones sobre propiedades e indices:
  - `obj.prop = ...`, `obj.prop += ...`
  - `lista[i] = ...`, `lista[i] += ...`
- Funciones de usuario (`func/endfunc`) con soporte de referencia `&`
- `return;` y `return expresion;`
- Modulos por archivo con `import modulo.submodulo;`
  (resuelve a `modulo/submodulo.clot` relativo al archivo actual)
- `import math` con builtin `sum(a, b)` como modulo nativo

Soportado en modo compile LLVM:
- Nucleo numerico (`=`, `+=`, `-=`, `if`, operadores, `print` numerico/string literal)
- `import math` y llamada `sum(a,b)` en expresiones numericas
- Funciones de usuario AOT (`func/endfunc`) sin retorno:
  - Llamadas como sentencia (`foo(...);`)
  - Parametros por valor y por referencia (`&`)
- Soporte completo del lenguaje via runtime bridge automatico
  (listas, objetos, funciones con `return`, referencias `&`, propiedades, indexacion, modulos)

No soportado aun en modo compile LLVM:
- AOT nativo de listas/objetos/indexacion/propiedades
- AOT nativo de `return` y funciones usadas como expresion
- AOT nativo de modulos de archivo
- Strings no literales como expresion general en AOT (fuera de `print("...")`)

## Visual Studio 2026 + WSL
Flujo recomendado:
1. Abrir carpeta del repo en Visual Studio.
2. Seleccionar toolchain de WSL (Ubuntu).
3. Usar CMake integrado con preset/manual:
   - `-DCLOT_ENABLE_LLVM=ON`
4. Compilar target `clot`.
5. Ejecutar con argumentos desde el perfil de depuracion:
   - `programa.clot --mode compile --emit exe -o programa`

## Documentacion de arquitectura
- `docs/architecture.md`

## Nota de compatibilidad
El refactor organiza la base para extender el compilador LLVM gradualmente. Si LLVM no esta instalado, `--mode compile` se deshabilita automaticamente y el binario sigue funcionando en modo interprete.
