# Benchmarks Baseline (Fase 0)

Objetivo: medir una linea base reproducible antes de optimizaciones fast-path.

## Ejecutar

```bash
benchmarks/baseline.sh ./build/wsl-release/clot
```

## Suite

- `numeric_loop.clot`: carga aritmetica en loop.
- `function_dispatch.clot`: costo de llamadas de funcion.
- `oop_dispatch.clot`: dispatch OOP (metodo de instancia).

## Notas

- Mide tiempos de modo interprete.
- Si LLVM esta disponible, el script tambien mide `--mode compile` para comparar.
- Los resultados son orientativos y dependen de hardware/entorno.
