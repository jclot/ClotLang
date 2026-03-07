#!/usr/bin/env bash
set -euo pipefail

BIN_PATH="${1:-./build/wsl-release/clot}"
PROGRAM_PATH="${2:-}"

if [[ -z "$PROGRAM_PATH" ]]; then
    echo "Uso: $0 <binario_clot> <programa.clot>" >&2
    exit 1
fi

if [[ ! -x "$BIN_PATH" ]]; then
    echo "Binario no encontrado o no ejecutable: $BIN_PATH" >&2
    exit 1
fi

if [[ ! -f "$PROGRAM_PATH" ]]; then
    echo "Programa no encontrado: $PROGRAM_PATH" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

INTERPRET_OUT="$TMP_DIR/interpret.out"
INTERPRET_ERR="$TMP_DIR/interpret.err"
COMPILE_LOG="$TMP_DIR/compile.log"
COMPILED_EXE="$TMP_DIR/program.exe"
COMPILED_OUT="$TMP_DIR/compile.out"
COMPILED_ERR="$TMP_DIR/compile.err"

set +e
"$BIN_PATH" "$PROGRAM_PATH" >"$INTERPRET_OUT" 2>"$INTERPRET_ERR"
INTERPRET_STATUS=$?
set -e

if ! "$BIN_PATH" "$PROGRAM_PATH" --mode compile --emit exe -o "$COMPILED_EXE" --verbose >"$COMPILE_LOG" 2>&1; then
    echo "Fallo compilacion LLVM para comparacion diferencial." >&2
    cat "$COMPILE_LOG" >&2
    exit 1
fi

set +e
"$COMPILED_EXE" >"$COMPILED_OUT" 2>"$COMPILED_ERR"
COMPILED_STATUS=$?
set -e

if [[ "$INTERPRET_STATUS" -ne "$COMPILED_STATUS" ]]; then
    echo "Diferencia de exit code: interpret=$INTERPRET_STATUS compile=$COMPILED_STATUS" >&2
    echo "--- interpret stderr ---" >&2
    cat "$INTERPRET_ERR" >&2
    echo "--- compile stderr ---" >&2
    cat "$COMPILED_ERR" >&2
    exit 1
fi

if ! diff -u "$INTERPRET_OUT" "$COMPILED_OUT" >/dev/null; then
    echo "Diferencia en stdout entre interpret y compile." >&2
    echo "--- interpret stdout ---" >&2
    cat "$INTERPRET_OUT" >&2
    echo "--- compile stdout ---" >&2
    cat "$COMPILED_OUT" >&2
    exit 1
fi

if ! diff -u "$INTERPRET_ERR" "$COMPILED_ERR" >/dev/null; then
    echo "Diferencia en stderr entre interpret y compile." >&2
    echo "--- interpret stderr ---" >&2
    cat "$INTERPRET_ERR" >&2
    echo "--- compile stderr ---" >&2
    cat "$COMPILED_ERR" >&2
    exit 1
fi

echo "Differential test OK: interpret y compile son equivalentes en salida y codigo de retorno."
