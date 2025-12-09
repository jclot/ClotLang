# The Clot Programming Language

Clot es un lenguaje de programación imperativo y en pleno desarrollo pensado para experimentos de matemáticas, física e inteligencia artificial. El intérprete actual está escrito en C++20 y se distribuye bajo licencia MIT, con un conjunto compacto de características que priorizan la legibilidad tipo Python y un flujo de ejecución sencillo.

## Tabla de contenidos
- [Estado del proyecto](#estado-del-proyecto)
- [Compilación y ejecución](#compilación-y-ejecución)
- [Sintaxis básica](#sintaxis-básica)
- [Tipos de datos admitidos](#tipos-de-datos-admitidos)
- [Expresiones y operadores](#expresiones-y-operadores)
- [Asignaciones y mutación](#asignaciones-y-mutación)
- [E/S: `print`](#es-print)
- [Funciones](#funciones)
- [Condicionales](#condicionales)
- [Módulos](#módulos)
- [Comentarios](#comentarios)
- [Ejemplo completo](#ejemplo-completo)
- [Estructura del repositorio](#estructura-del-repositorio)

## Estado del proyecto
El intérprete procesa archivos línea por línea y ejecuta las instrucciones de forma inmediata. Aún no hay soporte para bucles, pero ya se incluyen tipos numéricos, booleanos, listas, objetos, funciones con paso por referencia, condicionales anidados e importación básica de módulos. Las salidas de diagnóstico se imprimen en consola para ayudar a depurar.

## Compilación y ejecución
1. **Requisitos:** `g++` con soporte C++20.
2. **Compilar:**
   ```bash
   make
   ```
   Genera `ClotProgrammingLanguage/clot.exe` a partir de los fuentes del intérprete.【F:Makefile†L3-L31】
3. **Ejecutar el programa de ejemplo:**
   ```bash
   make run
   ```
   Ejecuta el intérprete sobre `ClotProgrammingLanguage/test.clot`. Usa `make build-run` para compilar y correr en una sola orden.【F:Makefile†L17-L25】

> Nota: El `Makefile` mantiene compatibilidad con la convención de nombres de Visual Studio, pero funciona en entornos Unix/Antigravity con `g++`.

## Sintaxis básica
- Cada instrucción termina con `;` (incluidas las llamadas a funciones y `print`).
- El intérprete ignora líneas vacías y comentarios que comienzan con `//`.
- Las operaciones se evalúan de izquierda a derecha respetando precedencia matemática estándar.

## Tipos de datos admitidos
- **Números de punto flotante (`double`)**: valor por defecto para expresiones numéricas.
- **Enteros (`int`)**: se almacenan automáticamente cuando una asignación numérica no contiene punto decimal.
- **Enteros largos (`long`)**: declarados con el prefijo `long` (por ejemplo, `long big = 9000;`). Validan rango de `long long` al asignar.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L18-L47】
- **Bytes (`byte`)**: valores entre 0 y 255 declarados con `byte nombre = valor;`。【F:ClotProgrammingLanguage/VariableAssignment.cpp†L18-L47】
- **Cadenas (`string`)**: literales entre comillas dobles o concatenaciones con `+`.
- **Booleanos (`true`/`false`)**: también pueden resultar de expresiones lógicas y se tratan como `1.0` o `0.0` en evaluaciones numéricas.【F:ClotProgrammingLanguage/ExpressionEvaluator.cpp†L18-L73】
- **Listas**: `[valor1, valor2, ...]` con números, strings o booleanos.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L51-L79】
- **Objetos**: `{clave: valor, ...}` con valores numéricos, strings, booleanos o listas.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L80-L109】

## Expresiones y operadores
El evaluador soporta aritmética y comparaciones con la precedencia mostrada (de mayor a menor):
1. Potencia `^`
2. Multiplicación `*`, división `/`, módulo `%`
3. Suma/resta `+`, `-`
4. Comparaciones `==`, `!=`, `<`, `<=`, `>`, `>=`
5. Lógico `&&`, `||`, `!`

Las expresiones combinan literales numéricos/booleanos, variables definidas y los operadores anteriores. Los resultados booleanos se devuelven como `1.0` (verdadero) o `0.0` (falso).【F:ClotProgrammingLanguage/ExpressionEvaluator.cpp†L18-L64】【F:ClotProgrammingLanguage/ExpressionEvaluator.cpp†L66-L103】

### Expresiones de strings
Las cadenas solo admiten concatenación con `+`. Se pueden mezclar literales, variables string y valores numéricos, que se convierten a texto al vuelo.【F:ClotProgrammingLanguage/ExpressionEvaluator.cpp†L105-L154】

## Asignaciones y mutación
- **Asignación simple:** `x = 1 + 2;`
- **Asignación compuesta:** `x += 3;` o `x -= 2;` (solo sobre variables numéricas ya definidas).【F:ClotProgrammingLanguage/VariableAssignment.cpp†L124-L145】
- **Listas y objetos:** se construyen directamente con `[]` o `{}` dentro de la asignación.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L51-L109】
- **Tipos explícitos:** `long` y `byte` requieren la forma `long nombre = 42;` o `byte flag = 1;` y validan el rango al asignar.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L18-L47】

El intérprete imprime trazas informativas tras cada asignación para facilitar la depuración.【F:ClotProgrammingLanguage/VariableAssignment.cpp†L18-L145】

## E/S: `print`
La función estándar `print(expr);` acepta:
- Literales string o booleanos.
- Cualquier variable numérica, string, booleana, lista u objeto.
- Expresiones numéricas o de cadenas (detecta automáticamente si debe evaluar aritmética o concatenar texto).【F:ClotProgrammingLanguage/PrintStatement.cpp†L9-L116】【F:ClotProgrammingLanguage/PrintStatement.cpp†L117-L138】

Las listas y objetos se formatean mostrando su contenido entre corchetes o llaves, respectivamente.【F:ClotProgrammingLanguage/PrintStatement.cpp†L37-L87】

## Funciones
### Declaración
```
func nombre(par1, &par2):
        // cuerpo indentado con tabuladores
endfunc
```
- Los parámetros marcados con `&` se pasan **por referencia**; el cuerpo puede modificar la variable del llamador.【F:ClotProgrammingLanguage/FunctionDeclaration.cpp†L5-L57】【F:ClotProgrammingLanguage/FunctionExecution.cpp†L28-L83】
- El cuerpo se recopila hasta encontrar `endfunc`; las líneas deben comenzar con tabulación para indicar pertenencia.【F:ClotProgrammingLanguage/FunctionDeclaration.cpp†L33-L57】

### Llamada
```
nombre(arg1, arg2);
```
- El número de argumentos debe coincidir con la definición.
- Los parámetros por referencia exigen un identificador existente en el contexto de llamada; se reflejan los cambios al finalizar la función.【F:ClotProgrammingLanguage/FunctionExecution.cpp†L28-L116】
- Dentro del cuerpo pueden usarse asignaciones, `print`, llamadas anidadas y condicionales; se restauran las variables numéricas locales al salir, excepto las pasadas por referencia.【F:ClotProgrammingLanguage/FunctionExecution.cpp†L85-L139】

## Condicionales
```
if condicion:
        // bloque if
else:
        // bloque else (opcional)
endif
```
- Las condiciones se evalúan como expresiones numéricas/booleanas; cualquier valor distinto de `0.0` se considera verdadero.【F:ClotProgrammingLanguage/ConditionalStatement.cpp†L1-L78】【F:ClotProgrammingLanguage/ConditionalStatement.cpp†L142-L168】
- Soporta `if`/`else` anidados tanto en el archivo principal como dentro de funciones.
- Cada rama se almacena y luego se ejecuta línea a línea, permitiendo asignaciones, `print`, importaciones, llamadas a funciones u otros condicionales.【F:ClotProgrammingLanguage/ConditionalStatement.cpp†L80-L140】【F:ClotProgrammingLanguage/ConditionalStatement.cpp†L170-L215】

## Módulos
La instrucción `import nombre;` carga funciones auxiliares.
- **math:** agrega la función `sum(a, b)` que retorna la suma de dos números.【F:ClotProgrammingLanguage/ModuleImporter.cpp†L5-L35】
- Las importaciones se validan y generan un error en caso de módulos desconocidos o falta del `;`.【F:ClotProgrammingLanguage/ModuleImporter.cpp†L5-L21】

## Comentarios
Las líneas que comienzan con `//` se tokenizan como comentarios y se ignoran durante la ejecución.【F:ClotProgrammingLanguage/Tokenizer.cpp†L15-L26】

## Ejemplo completo
```clot
// Variables y tipos
long max = 9000;
byte level = 42;
flag = true;
nums = [1, 2, 3];
user = {name: "Ada", active: true, scores: nums};

// Función con referencia
func inc(&value):
        value += 1;
endfunc

// Condicional
if(flag && max > 100):
        print("User: " + user.name + " level " + level);
        inc(max);
else:
        print("Disabled");
endif

// Uso de módulo
import math;
print(sum(5, 7));
```

## Estructura del repositorio
- `ClotProgrammingLanguage/`: código fuente del intérprete y binario resultante `clot.exe`.
- `ClotProgrammingLanguage/test.clot`: script de ejemplo ejecutado por defecto en `main()`.
- `TokenizerTests/`: espacio reservado para pruebas unitarias de tokenización.
- `Makefile`: reglas de compilación y ejecución con `g++`.
- `LICENSE.txt`: licencia MIT.

Este README refleja el estado actual del lenguaje; las nuevas capacidades (bucles, más tipos, módulos adicionales) se incorporarán en futuras versiones.
