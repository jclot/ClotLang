# RFC-OOP-MVP

- Estado: aceptado (Fase 0)
- Fecha: 2026-03-05
- Alcance: Fase 1 (interprete)

## Objetivo

Agregar OOP MVP a Clot sin romper el modelo dinamico actual ni el runtime bridge de LLVM.

## Sintaxis aprobada

- `interface Nombre: ... endinterface`
- `class Nombre [extends Base] [implements I1, I2]: ... endclass`
- `constructor(...): ... endconstructor`
- `public|private`, `static`, `readonly`, `override`
- `get nombre(): ... endget`
- `set nombre(value[: tipo]): ... endset`
- metodos: `func [tipo] nombre(...): ... endfunc`

## Semantica aprobada (MVP)

1. Herencia simple: solo `extends` de una clase.
2. Interfaces: solo firmas; `implements` exige metodos compatibles.
3. `override` obligatorio cuando se redefine metodo heredado.
4. `private` visible solo en contexto de clase propietaria.
5. `readonly`:
- instancia: solo asignable en constructor de su clase propietaria.
- static: no modificable tras declaracion.
6. Campos con type hint validan en runtime asignaciones/mutaciones.
7. Constructor:
- sin constructor explicito: constructor vacio.
- con `extends`, el constructor derivado debe invocar `super(...)` como primera sentencia.
- `super(...)` solo una vez por constructor.
8. `super.metodo(...)` permitido en metodos de clase con contexto de instancia.

## Compatibilidad con LLVM

- `--mode compile` no genera AOT nativo para OOP MVP en esta fase.
- Programas con OOP usan runtime bridge automaticamente para mantener semantica completa.

## Fuera de alcance

- `protected`, `abstract`, `final`, `sealed`.
- herencia multiple de clases.
- reflection/metaclasses.
