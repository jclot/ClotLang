# Clot Multiparadigm Blueprint (Plan Seleccionado)

Fecha: 2026-03-04  
Estado: propuesta estrategica y tecnica (sin cambios de codigo en este archivo)

## 1) Decision final: mejor opcion para Clot

Plan elegido: **Opcion D+ (Evolucion por capas, con OOP + funcional + ruta rapida tipada en paralelo)**.

Por que esta es la mejor opcion para Clot ahora:

1. Respeta la base actual del repo (interprete dinamico + LLVM AOT + runtime bridge).
2. Evita un "big bang" que rompa semantica y tooling.
3. Permite entregar valor temprano sin sacrificar seguridad ni performance futura.
4. Da una ruta realista para acercarse a:
   - ergonomia OOP estilo Java (estructura, contratos, encapsulacion),
   - expresividad funcional estilo Python/JS (closures, HOF, pipelines),
   - performance cercana a C en rutas tipadas/hot paths,
   - potencia matematica estilo MATLAB (vectorizacion + backend numerico).

## 2) North Star de lenguaje

Clot debe converger a:

- **Multiparadigma pragmatica**: imperativo + OOP + funcional, sin dogma.
- **Tipado hibrido**: dinamico por defecto, tipado opcional fuerte donde aporte.
- **Regla explicita**: declarar tipos no es obligatorio; el tipado es opt-in.
- **Dual execution**:
  - flexible path: interpret / runtime bridge,
  - fast path: AOT/JIT especializado para codigo estable.
- **Safe-by-default**: seguridad y controles de capacidades desde el diseno.

## 3) Lo que SI se implementa primero (alcance realista)

## MVP OOP (primera entrega)

- `class`, `endclass`
- atributos de instancia y estaticos
- metodos de instancia y estaticos
- `constructor`, `endconstructor`
- `this`
- `extends` (herencia simple, una sola clase base)
- `interface`, `endinterface`
- `implements` (una o multiples interfaces)
- `get` / `set` (propiedades)
- modificadores MVP:
  - `public` (default)
  - `private`
  - `static`
  - `readonly`
  - `override` (obligatorio cuando redefines metodo heredado)

## MVP funcional

- lambdas
- closures lexicales
- funciones de orden superior (`map`, `filter`, `reduce`, `flat_map`)
- composicion simple y pipelines en stdlib (sin sistema de efectos aun)

## Performance MVP

- especializacion de funciones tipadas "hot" (sin cubrir todo el lenguaje)
- mantener runtime bridge para casos no soportados AOT

## 4) Diseno OOP detallado (propuesta de sintaxis y semantica)

## 4.1 Sintaxis base

```clot
interface Saludo:
    func string saludar(prefijo: string = "Hola");
endinterface

class Persona implements Saludo:
    private string _nombre;
    private int _edad = 0;
    public static int total = 0;
    public readonly string id;

    constructor(nombre: string, edad: int = 0):
        this._nombre = nombre;
        this._edad = edad;
        this.id = "P-" + str(Persona.total + 1);
        Persona.total += 1;
    endconstructor

    public func string saludar(prefijo: string = "Hola"):
        return prefijo + ", soy " + this._nombre;
    endfunc

    public get nombre():
        return this._nombre;
    endget

    public set nombre(value: string):
        this._nombre = value;
    endset
endclass
```

## 4.2 Modificadores (que incluir y cuando)

## MVP (fase inicial)

- `public`: visible externamente.
- `private`: visible solo dentro de la clase.
- `static`: miembro de clase, no de instancia.
- `readonly`: asignable solo en declaracion o constructor.
- `override`: obligatorio para sobrescritura de metodo heredado.

## Fase siguiente (cuando MVP este estable)

- `protected` (visible en subclases)
- `abstract` (clases/metodos incompletos)
- `final` (evita override o herencia)
- `sealed` (jerarquia cerrada para seguridad/mantenibilidad)

## 4.3 Clases

Reglas:

1. Una clase puede extender solo una clase base (`extends` simple).
2. Puede implementar multiples interfaces.
3. Si no hay constructor declarado, se genera constructor vacio.
4. Si hay `extends`, el constructor base debe ser invocado via `super(...)`.
5. No hay herencia multiple de clases (evita complejidad MRO).

## 4.4 Atributos

Reglas:

1. Atributos de instancia viven por objeto.
2. Atributos `static` viven en la clase.
3. `readonly`:
   - se permite asignar al declarar,
   - o dentro del constructor de esa clase,
   - fuera de eso: error runtime/analyzer.
4. Si se define type hint en atributo, se valida asignacion runtime.

Ejemplo:

```clot
class Config:
    public readonly string app_name = "clot-app";
    private int _port = 8080;
endclass
```

## 4.5 Metodos

Reglas:

1. Metodos de instancia reciben `this`.
2. Metodos `static` se llaman por clase (`Clase.metodo()`).
3. `override` obligatorio en metodo reescrito.
4. Firmas con defaults y type hints siguen mismas reglas ya existentes.

Ejemplo:

```clot
class MathEx:
    public static func int doble(x: int):
        return x * 2;
    endfunc
endclass
```

## 4.6 Constructor

Reglas:

1. Se define con `constructor(...): ... endconstructor`.
2. No retorna valor.
3. Puede usar parametros con defaults y type hints.
4. En clases con `extends`, `super(...)` debe ejecutarse antes de usar `this`.

## 4.7 Extends

Reglas:

1. Solo herencia simple.
2. Metodos heredados pueden sobrescribirse con `override`.
3. Compatibilidad de firma en override:
   - misma aridad minima/maxima,
   - tipos compatibles en hints (cuando existan),
   - retorno compatible.

Ejemplo:

```clot
class Animal:
    public func string sonido():
        return "???";
    endfunc
endclass

class Perro extends Animal:
    public override func string sonido():
        return "guau";
    endfunc
endclass
```

## 4.8 Interfaces

Reglas:

1. Solo contratos (firmas), sin estado en MVP.
2. `implements` exige implementar todos los metodos.
3. Error de analyzer/runtime si falta implementacion.
4. Se soporta multiple interface implementation.

## 4.9 Get / Set

Objetivo: encapsulacion limpia sin sacrificar ergonomia.

Reglas:

1. `get` define lectura controlada de propiedad.
2. `set` define escritura controlada.
3. Se recomienda backing field privado (`_name`, `_age`, etc.).
4. Si solo hay `get`, propiedad de solo lectura externa.

Ejemplo:

```clot
class Cuenta:
    private decimal _saldo = 0.0;

    public get saldo():
        return this._saldo;
    endget

    public set saldo(value: decimal):
        if value < 0:
            throw("saldo invalido");
        endif
        this._saldo = value;
    endset
endclass
```

## 5) Diseno funcional detallado

## 5.1 Lambdas y closures

Sintaxis propuesta:

```clot
inc = (x) => x + 1;
make_adder = (a) => (b) => a + b;
add10 = make_adder(10);
println(add10(5)); // 15
```

Reglas:

1. Captura lexical por cierre (closure).
2. Captura por referencia para mutables, por valor para inmutables pequeñas (decision de runtime).
3. Analyzer advierte closures con side effects en pipelines "puros".

## 5.2 HOF de colecciones

MVP stdlib:

- `map(list, fn)`
- `filter(list, pred)`
- `reduce(list, seed, fn)`
- `flat_map(list, fn)`

Objetivo:

- composicion declarativa de transformaciones,
- menor codigo boilerplate imperativo.

## 6) Tipos: modelo hibrido (dinamico + hints + clases)

## 6.1 Estado objetivo

1. Dinamico por defecto (compatible con Clot actual).
2. Type hints opcionales en funciones, metodos, atributos.
3. Clases e interfaces como tipos validables.
4. Especificar tipo sigue siendo no obligatorio para preservar dinamismo y ergonomia.

## 6.2 Reglas clave

1. Sin hint: comportamiento dinamico.
2. Con hint: validacion runtime obligatoria.
3. `--mode analyze` verifica:
   - override signatures,
   - implementaciones de interfaces,
   - asignaciones a `readonly`,
   - errores obvios de visibilidad.

Ejemplo (convivencia dinamico + tipado):

```clot
func procesar(a, b):        // dinamico, sin tipos obligatorios
    return a + b;
endfunc

func int sumar(a: int, b: int): // tipado opt-in
    return a + b;
endfunc
```

## 7) Seguridad integrada al diseno

## 7.1 Seguridad de lenguaje

1. Enforce real de `private`/`protected` (no solo convencion).
2. Sin reflection insegura en MVP.
3. Sin eval arbitrario en core.
4. Constructor seguro: objeto no usable hasta terminar inicializacion.

## 7.2 Seguridad de runtime/sistema

1. Capabilities por modulo (archivo/proceso/red).
2. Limites opcionales de memoria, recursion y tiempo.
3. Auditoria de builtins I/O.
4. Fuzzing parser + evaluator + builtins criticos.

## 8) Performance: como acercarse a C y MATLAB sin vender humo

## 8.1 Regla de oro

- No prometer velocidad C para todo.
- Si prometer rendimiento alto en rutas tipadas y estables.

## 8.2 Estrategia tecnica

1. Detectar funciones hot (profiling + hints).
2. Especializar por firma (tipos estables).
3. Inlining y optimizaciones LLVM para loops numericos.
4. Backend numerico:
   - `vector`/`matrix`,
   - broadcasting,
   - BLAS/LAPACK para operaciones densas.

## 8.3 Meta de rendimiento (prediccion razonable)

- scripts dinamicos generales: mejora incremental (1.2x-2x por iteracion de optimizacion).
- kernels tipados: 3x-20x vs ruta interpretada.
- algebra lineal via backend nativo: orden de magnitud mayor que loops dinamicos.

## 9) Roadmap detallado (implementacion real en este repo)

## Fase 0 (2-4 semanas): especificacion y base

- RFC de sintaxis OOP/funcional.
- Definir AST nuevos.
- test diferencial interpret vs compile.
- benchmark baseline.

Archivos impactados:

- `include/clot/frontend/ast.hpp`
- `src/frontend/parser_*.cpp`
- `src/frontend/static_analyzer.cpp`

## Fase 1 (4-8 semanas): OOP MVP en interprete

- parser de `class/interface/constructor/get/set`.
- runtime object model (metadata de clase + slots de instancia).
- dispatch de metodos.
- visibilidad y readonly enforcement.

Archivos impactados:

- `src/interpreter/interpreter.cpp`
- `src/interpreter/interpreter_state.cpp`
- `src/runtime/value.hpp` (+ impl)

## Fase 2 (4-8 semanas): funcional MVP

- lambdas y closures.
- HOF en stdlib.
- analyzer para captures y warnings utiles.

## Fase 3 (6-10 semanas): compile path y fast path

- lowering parcial de clases/metodos y lambdas a LLVM cuando sea viable.
- fallback runtime bridge para casos no cubiertos.
- especializacion de funciones tipadas hot.

## Fase 4 (6-12 semanas): numerica estilo MATLAB

- `vector`/`matrix`.
- ops vectorizadas.
- integracion BLAS/LAPACK.
- pruebas numericas y de precision.

## 10) Matriz de pruebas obligatoria

## Parser

- declaraciones validas/invalidas de clases, interfaces y modifiers.
- errores de sintaxis con linea/columna correctas.

## Analyzer

- override sin `override` -> error.
- clase que no implementa interfaz -> error.
- acceso `private` externo -> error.
- write a `readonly` fuera de constructor -> error.

## Runtime

- dispatch correcto en herencia.
- `super(...)` y orden de init.
- getters/setters con validaciones.
- closures capturando estado.

## Compile/Bridge

- equivalencia de resultados interpret vs compile.
- fallback bridge cuando AOT no cubra feature.
- regresion en `tests/smoke.sh` y `tests/llvm_smoke.sh`.

## Seguridad

- bypass de visibilidad rechazado.
- validacion de capacidades en builtins sensibles.
- fuzzing basico parser/evaluator.

## 11) Criterios de "go/no-go" por fase

Fase se considera completada solo si:

1. Semantica documentada + tests verdes.
2. Cero regresiones criticas en features existentes.
3. Mensajes de error consistentes ES/EN.
4. Dif interpret/compile en test diferencial <= umbral acordado.

## 12) Lo que NO se hara al inicio (para proteger calidad)

- herencia multiple de clases
- metaclasses
- reflection completa
- generics avanzados de tipo compilador
- sistema de efectos completo
- promesa de "C-speed" para flujo 100% dinamico

## 13) Alternativas consideradas y por que no fueron la mejor

- Big Bang multiparadigma total: demasiado riesgo de deuda y regresiones.
- Solo OOP primero: retrasa demasiado data pipelines funcionales.
- Solo funcional primero: deja corto el modelado empresarial.

Por eso D+ es la mejor decision para Clot hoy.

## 14) Prediccion a 12 meses (si se ejecuta este plan)

Escenario base esperado:

1. OOP MVP estable y usable en proyectos reales.
2. Funcional MVP maduro para transformaciones de datos.
3. Ruta tipada con mejoras claras de rendimiento en kernels.
4. Primer stack numerico serio (vector/matrix + BLAS wrappers).
5. Seguridad operativa mejor que baseline actual (capabilities + enforcement de visibilidad).

## 15) Proxima accion recomendada inmediata

Crear y aprobar 3 RFCs antes de codificar:

1. RFC-OOP-MVP (sintaxis + semantica exacta de class/interface/modifiers/get/set).
2. RFC-FUNC-MVP (lambdas, closures, HOF, reglas de captura).
3. RFC-FASTPATH-001 (criterios de especializacion y fallback bridge).
