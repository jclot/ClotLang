# RFC-FASTPATH-001

- Estado: borrador base (Fase 0)
- Fecha: 2026-03-05
- Alcance: fases posteriores (no implementado en Fase 1)

## Objetivo

Definir reglas de especializacion para acercar rendimiento en rutas tipadas/hot sin romper dinamismo.

## Principios

1. Correctitud primero: fallback al runtime bridge ante dudas.
2. Especializacion por firma estable (tipos + aridad).
3. Medicion obligatoria con benchmarks reproducibles.

## Criterios iniciales

- Solo especializar funciones con perfil estable.
- Invalidar especializacion al detectar desviacion de tipos.
- Mantener equivalencia observable con modo interpret.

## Entregables base de Fase 0

- script diferencial interpret vs compile.
- baseline de benchmarks numericos y de dispatch.
