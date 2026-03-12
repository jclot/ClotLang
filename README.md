# Clot Programming Language

Refactor de arquitectura para separar claramente frontend, runtime, interprete y backend LLVM.

## Estado actual
- Nuevo CLI unificado: `clot`.
- Nuevo frontend (tokenizador + parser + AST) en `include/clot/frontend` y `src/frontend`.
- Nuevo interprete modular en `include/clot/interpreter` y `src/interpreter`.
- Backend LLVM para compilacion AOT (`--mode compile`) en `include/clot/codegen` y `src/codegen`.
- Internacionalizacion basica de interfaz y diagnosticos (`--lang es|en`, `CLOT_LANG`).
- Tipado opcional en funciones y parametros (incluye `any`/`dynamic` como alias).
- OOP MVP en modo interprete: `class/interface`, `extends`, `implements`, `constructor`, `get/set`, `public/private/static/readonly/override`, `super(...)` y `super.metodo(...)`.
- Migracion funcional base completada en modo interprete: listas, objetos, indexacion, propiedades, funciones y `import math`.
- Builtins utilitarios nuevos en runtime: `len`, `range`, `enumerate`, `zip`, `all`, `any`, `isinstance`, `chr`, `ord`, `hex`, `bin`, `hash`, `id`.

## Perfil del lenguaje
- Multiparadigma (dominante: imperativo/procedural).
- OOP MVP disponible (encapsulacion, herencia simple, contratos por interfaces).
- Funcional ligero (funciones de primera clase), pero no funcional puro.
- Ejecucion dual:
  - Interpretado: `--mode interpret`
  - Compilado AOT LLVM: `--mode compile`
- Tipado dinamico con hints opcionales:
  - Declaraciones y type hints se validan en runtime.
  - Fase 1: tipado de contenedor en colecciones (`list` / `object`).
  - Fase 2: tipado fuerte interno con anotaciones genericas (`list<int>`, `tuple<float>`, `set<char>`, `map<string, int>`, `object<int>`; admite anidacion).
  - `--mode analyze` agrega chequeo estatico auxiliar (no reemplaza tipado estatico completo).
- El tipado explicito sigue siendo opt-in: puedes combinar estilo dinamico y tipado por hints.

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
  install.sh
  install.ps1
  install_llvm_wsl.sh
  uninstall.sh
  uninstall.ps1
CMakeLists.txt
Makefile
```

## Requisitos
- Ubuntu 24.04+ en WSL2.
- `cmake` 3.20+.
- `ninja` recomendado.
- LLVM/Clang para usar `--mode compile`.

## Instalacion rapida (usuarios finales)
Linux/macOS (instala en `~/.local/bin`):
```bash
curl -fsSL https://raw.githubusercontent.com/jclot/ClotLang/main/scripts/install.sh | bash
```

Windows PowerShell (instala en `%LOCALAPPDATA%\Clot\bin`):
```powershell
iwr -useb https://raw.githubusercontent.com/jclot/ClotLang/main/scripts/install.ps1 | iex
```

Despues de instalar:
```bash
clot programa.clot
```

Opcional:
- Fijar version Linux/macOS: `CLOT_VERSION=v0.2.0`
- Fijar version Windows PowerShell: `$env:CLOT_VERSION="v0.2.0"`
- Cambiar prefijo Linux/macOS: `CLOT_PREFIX=/ruta/personal`
- Cambiar ruta Windows: `$env:CLOT_INSTALL_DIR="C:\Ruta\Clot"`

Notas:
- Los instaladores descargan assets desde GitHub Releases con nombres como `clot-linux-x86_64.tar.gz`, `clot-macos-arm64.tar.gz`, `clot-windows-x86_64.zip`.
- Los binarios de release se publican sin LLVM (solo modo interprete). Para `--mode compile`, compila desde fuente con LLVM.

Verificar checksum (opcional):
- Linux/macOS: `shasum -a 256 clot-linux-x86_64.tar.gz`
- Windows: `Get-FileHash clot-windows-x86_64.zip -Algorithm SHA256`

Desinstalar:
- Linux/macOS: `bash scripts/uninstall.sh`
- Windows PowerShell: `iwr -useb https://raw.githubusercontent.com/jclot/ClotLang/main/scripts/uninstall.ps1 | iex`

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

## Compilar (CMake)
Opcion A (recomendada, usando presets WSL):
```bash
cmake --preset wsl-release
cmake --build --preset build-release -j4
```
Binario generado: `./build/wsl-release/clot`

Opcion B (directa, sin presets):
```bash
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCLOT_ENABLE_LLVM=ON
cmake --build build -j4
```
Binario generado: `./build/clot`

Notas sobre paralelismo (`-j` / `--parallel`):
- `cmake --build ...` -> build normal (sin forzar paralelismo).
- `cmake --build ... -j4` -> ejecuta 4 tareas de compilacion en paralelo.
- `cmake --build ... -j8` -> igual, pero con 8 tareas.
- `cmake --build ... --parallel` -> usa paralelismo automatico segun el sistema.
- `cmake --build ... --parallel 6` -> fija 6 tareas en paralelo.

Alternativa:
```bash
make
```

## Uso
### Modo interprete
```bash
./build/clot programa.clot
```

Si compilaste con presets, usa:
```bash
./build/wsl-release/clot programa.clot
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

Comparacion diferencial interpret vs compile (incluye fallback runtime bridge):
```bash
scripts/diff_interpret_compile.sh ./build/wsl-release/clot examples/basic.clot
```

Baseline de benchmarks (Fase 0):
```bash
benchmarks/baseline.sh ./build/wsl-release/clot
```

## Ejemplos rapidos
```clot
const long n = 5;
sum = 0;

for (int i = 0; i < n; i++):
    if(i == 2):
        continue;
    endif
    sum += i;
endfor

for (item in [10, 20, 30]):
    println(item);
endfor

switch(sum):
    case 8:
        println("ok");
        break;
    default:
        println("otro");
endswitch

do:
    sum -= 1;
while(sum > 0);

try:
    throw("boom");
catch(err):
    println(err);
finally:
    println("cleanup");
endtry

func demo():
    defer println("fin de bloque");
    println(3 in [1, 2, 3]);
endfunc
```

## Sintaxis de control de flujo
```clot
for (int i = 0; i < n; i++):
    println(i);
endfor

for (const item in coleccion):
    println(item);
endfor

switch(valor):
    case 1:
        println("uno");
        break;
    default:
        pass;
endswitch

do:
    trabajo();
while(condicion);

try:
    trabajo_riesgoso();
catch(Error err):
    println(err);
finally:
    println("cleanup");
endtry

defer println("al salir del bloque");
```

- `for (init; cond; update):` conserva cabecera tipo Java, pero el bloque sigue usando `:` y `endfor`.
- `init` y `update` aceptan declaraciones, mutaciones o expresiones simples; no aceptan bloques ni control de flujo.
- `for (item in coleccion):` usa `in` para iterar listas, tuple, set, map, object o string; el binding puede ser `const` y/o tipado.
- `switch` soporta `case`, `default`, fallthrough estilo Java y `break;` para cortar el flujo.
- `break;` sale del bucle o `switch` mas cercano. `continue;` solo aplica a bucles, incluso si aparece dentro de un `switch` anidado.
- `pass;` no hace nada y sirve como placeholder valido.
- `defer` solo acepta sentencias simples y se ejecuta en orden LIFO al salir del bloque actual.
- `finally` siempre se ejecuta al cerrar el `try`, haya excepcion capturada, no capturada o retorno temprano.
- `in` tambien funciona como operador de pertenencia: `clave in mapa`, `item in lista`, `"na" in "banana"`.

## Alcance actual
Soportado en modo interprete:
- Asignaciones numericas (`=`, `+=`, `-=`)
- Declaraciones `long` y `byte` (tratadas como numericas)
- `print(...)` de expresiones numericas y string
- `if / else / endif`
- `switch/case/default/endswitch` (con fallthrough estilo Java y `break;`)
- Bucles y control de flujo:
  - `while/endwhile`
  - `for/endfor` clasico: `for (int i = 0; i < n; i++):`
  - `for-each`: `for (item in coleccion):`
  - `do ... while(condicion);`
  - `break;`, `continue;`, `pass;`
- Operadores aritmeticos, comparacion y logicos
- Operador de pertenencia `in` (listas, tuple, set, map, object, string)
- `const` para declarar valores inmutables
- Listas y objetos (incluye `nums[2]` y `user.name`)
- Mutaciones sobre propiedades e indices:
  - `obj.prop = ...`, `obj.prop += ...`
  - `lista[i] = ...`, `lista[i] += ...`
- Funciones de usuario (`func/endfunc`) con soporte de referencia `&`
- `return;` y `return expresion;`
- Type hints opcionales en funciones:
  - Retorno: `func float promedio(...):`
  - Parametros: `func greet(name: string):`
  - Genericos en retorno/parametros: `func list<int> filtrar(xs: list<int>):`
  - Valores por defecto: `func f(x: float = 1.0):`
  - Retorno tipado: exactamente 1 tipo por funcion (sin unions)
  - Tipos permitidos en retorno/parametros: `int`, `double`, `float`, `decimal`, `long`, `byte`, `char`, `tuple`, `set`, `map`, `function`, `string`, `bool`, `list`, `object`, `null`
  - Aridad generica valida: `list<T>`, `tuple<T>`, `set<T>`, `object<T>`, `map<K, V>`
  - `any` y `dynamic` son alias (equivalentes)
  - Retorno dinamico: omitir tipo (`func nombre(...):`) o usar `func any nombre(...):` / `func dynamic nombre(...):`
  - En parametros: `x`, `x: any` y `x: dynamic` son equivalentes (aceptan cualquier valor)
  - `any/dynamic` en la doc significa "any o dynamic"; no se escribe literal con barra
  - Validacion runtime cuando el hint esta presente
- Tipado de colecciones en variables:
  - Contenedor: `list nums = [1, 2, 3];`, `object cfg = {enabled: true};`
  - Interno fuerte: `list<int> ids = [1, 2, 3];`, `map<string, int> scores = map("a", 10);`
  - Si algun elemento interno no coincide, falla en runtime con error tipado.
  - Aridad generica: `list<T>`, `tuple<T>`, `set<T>`, `object<T>`, `map<K, V>` (tambien se permite sin genericos por compatibilidad).

Ejemplos detallados de tipado de colecciones:

```clot
// Fase 1: tipado de contenedor
list datos = [1, "dos", true];
object meta = {id: 1, name: "Ana", ok: true};
```

```clot
// Fase 2: tipado interno fuerte
list<int> ids = [1, 2, 3];
tuple<float> coords = tuple(1, 2.5, 3);
set<char> letras = set('a', "b", 67);
map<string, int> score = map("ana", 10, "luis", 20);
object<int> counters = {ok: 5, fail: 1};
list<map<string, int>> tabla = [map("a", 1), map("b", 2)];
```

```clot
// Errores tipicos
list<int> bad1 = [1, "2"];                    // Elemento list[1] incompatible con 'int'
map<string, int> bad2 = map("ana", 10, 2, 20); // Clave map[1] incompatible con 'string'
object<int> bad3 = {ok: 1, label: "x"};       // Propiedad object.label incompatible con 'int'
```

```clot
// Funciones y for-each tipados
func list<int> keep(xs: list<int>):
    return xs;
endfunc

for (list<int> row in [[1, 2], [3, 4]]):
    println(row[0]);
endfor
```
- Modulos por archivo:
  - `import modulo.submodulo;`
  - `import modulo.submodulo as alias;`
  - `from modulo.submodulo import simbolo;`
  - `from modulo.submodulo import simbolo as alias;`
  (resuelve `modulo/submodulo.clot` y busca desde el directorio actual y sus ancestros, incluyendo raiz de proyecto)
- `import math` con builtin `sum(a, b)` como modulo nativo
- Builtins de colecciones/tipos estilo Python:
  - `len(value)` para `string/list/tuple/set/map/object/char`
  - `range(stop)`, `range(start, stop)`, `range(start, stop, step)` (retorna `list`, maximo 1,000,000 elementos)
  - `enumerate(iterable, start=0)` y `zip(iter1, iter2, ...)` (retornan `list` de `tuple`)
  - `all(iterable)` / `any(iterable)` para validaciones booleanas masivas
  - `isinstance(value, type_name)` con `type_name` string o coleccion de strings
  - `chr(code)` / `ord(char)` para conversion char <-> entero ASCII (0..255)
  - `hex(value)` / `bin(value)` para conversion de base con prefijos `0x`/`0b`
  - `hash(value)` para fingerprint estructural y `id(value)` para identidad runtime estable por valor
- Manejo de excepciones:
  - `throw(value);`
  - `try/catch/endtry` con formas: `catch:`, `catch(err):`, `catch(Tipo):`, `catch(Tipo err):`
  - `finally` opcional: `try/finally/endtry` o `try/catch/finally/endtry`
  - `try/catch` es opcional: si no hay `catch` compatible, termina con `Excepcion no capturada: <Tipo>: <mensaje>`
  - Tipos runtime base inferidos para errores internos: `RuntimeError`, `TypeError`, `ArgumentError`, `MissingArgumentError`, `TooManyArgumentsError`, `ValueError`, `RangeError`, `IndexError`, `NameError`, `AttributeError`, `IOError`, `FileNotFoundError`, `PermissionError`, `FileExistsError`, `FileClosedError`, `AssertionError`, `ImportError`, `ModuleNotFoundError`
- Ejecucion diferida con `defer` (LIFO al salir del bloque actual)
- Modulo base de excepciones en Clot: `import clot.core.exceptions;`
- OOP MVP:
  - `interface/endinterface` con firmas `func ...;`
  - `class/endclass` con `extends` (herencia simple) y `implements` (multiple)
  - `constructor/endconstructor`
  - `get/endget`, `set/endset`
  - modificadores: `public` (default), `private`, `static`, `readonly`, `override`
  - validaciones runtime:
    - `override` obligatorio al sobrescribir
    - compatibilidad de firma/retorno en override
    - contrato de interfaces (`implements`)
    - visibilidad `private`
    - `readonly` de instancia (solo constructor de la clase propietaria)
    - `readonly static`
    - `super(...)` obligatorio como primera sentencia en constructor derivado
    - `super(...)` maximo una vez por constructor

Soportado en modo compile LLVM:
- Nucleo numerico (`=`, `+=`, `-=`, `if`, operadores, `print` numerico/string literal)
- `import math` y llamada `sum(a,b)` en expresiones numericas
- Funciones de usuario AOT (`func/endfunc`) sin retorno:
  - Llamadas como sentencia (`foo(...);`)
  - Parametros por valor y por referencia (`&`)
- Soporte completo del lenguaje via runtime bridge automatico
  (listas, objetos, funciones con `return`, referencias `&`, propiedades, indexacion, modulos, OOP MVP)

No soportado aun en modo compile LLVM:
- AOT nativo de listas/objetos/indexacion/propiedades
- AOT nativo de `return` y funciones usadas como expresion
- AOT nativo de modulos de archivo
- AOT nativo de OOP (`class/interface/constructor/get/set/extends/implements/super`)
- AOT nativo de `switch`, `for-each`, `do-while`, `defer` y `finally`
- AOT nativo del operador de pertenencia `in`
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
