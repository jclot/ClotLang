# RFC-FUNC-MVP

- Estado: borrador base (Fase 0)
- Fecha: 2026-03-05
- Alcance: fases posteriores (no implementado en Fase 1)

## Objetivo

Definir la base funcional futura sin romper compatibilidad con el estilo imperativo/OOP.

## Direccion aprobada

1. Lambdas con cierre lexico.
2. Closures de primer orden.
3. HOF en stdlib (`map`, `filter`, `reduce`, `flat_map`).
4. Pipeline simple en stdlib.

## Reglas base

- Tipado sigue siendo opt-in (hints opcionales).
- Semantica dinamica por defecto.
- Camino AOT podra especializar rutas tipadas cuando sea seguro.

## Fuera de alcance inmediato

- sistema de efectos.
- pureza forzada.
- optimizacion funcional agresiva en LLVM.
