#!/usr/bin/env bash
set -euo pipefail

BIN_PATH="${1:-./build/wsl-release/clot}"

if [[ ! -x "$BIN_PATH" ]]; then
    echo "Binary no encontrado o no ejecutable: $BIN_PATH" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

cat > "$TMP_DIR/aot_functions.clot" <<'PROG'
import math;
max = 10;
byte delta = 2;

func bump(&value, amount):
    value += sum(amount, 3);
    if(value > 20):
        print("alto");
    else:
        print("bajo");
    endif
endfunc

bump(max, delta);
print(max);
PROG

AOT_EXE="$TMP_DIR/aot_functions"
AOT_LOG="$TMP_DIR/aot_functions.log"
"$BIN_PATH" "$TMP_DIR/aot_functions.clot" --mode compile --emit exe -o "$AOT_EXE" --verbose >"$AOT_LOG" 2>&1

if grep -q "runtime bridge LLVM activado" "$AOT_LOG"; then
    echo "Fallo llvm_smoke: este caso deberia compilar AOT sin runtime bridge." >&2
    cat "$AOT_LOG" >&2
    exit 1
fi

EXPECTED_AOT=$'bajo\n15'
ACTUAL_AOT="$($AOT_EXE)"
if [[ "$ACTUAL_AOT" != "$EXPECTED_AOT" ]]; then
    echo "Fallo llvm_smoke (AOT funciones)" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_AOT" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_AOT" >&2
    exit 1
fi

cat > "$TMP_DIR/aot_long_small.clot" <<'PROG'
import math;
long value = 42;
value += sum(1, 2);
print(value);
PROG

AOT_LONG_SMALL_EXE="$TMP_DIR/aot_long_small"
AOT_LONG_SMALL_LOG="$TMP_DIR/aot_long_small.log"
"$BIN_PATH" "$TMP_DIR/aot_long_small.clot" --mode compile --emit exe -o "$AOT_LONG_SMALL_EXE" --verbose >"$AOT_LONG_SMALL_LOG" 2>&1

if grep -q "runtime bridge LLVM activado" "$AOT_LONG_SMALL_LOG"; then
    echo "Fallo llvm_smoke: aot_long_small deberia compilar AOT puro." >&2
    cat "$AOT_LONG_SMALL_LOG" >&2
    exit 1
fi

EXPECTED_AOT_LONG_SMALL=$'45'
ACTUAL_AOT_LONG_SMALL="$($AOT_LONG_SMALL_EXE)"
if [[ "$ACTUAL_AOT_LONG_SMALL" != "$EXPECTED_AOT_LONG_SMALL" ]]; then
    echo "Fallo llvm_smoke (aot_long_small)" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_AOT_LONG_SMALL" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_AOT_LONG_SMALL" >&2
    exit 1
fi

cat > "$TMP_DIR/aot_long_near_max_ok.clot" <<'PROG'
long value = 9223372036854775800;
print(value);
PROG

AOT_LONG_OK_EXE="$TMP_DIR/aot_long_near_max_ok"
"$BIN_PATH" "$TMP_DIR/aot_long_near_max_ok.clot" --mode compile --emit exe -o "$AOT_LONG_OK_EXE" --verbose >"$TMP_DIR/aot_long_near_max_ok.log" 2>&1

set +e
"$AOT_LONG_OK_EXE" >"$TMP_DIR/aot_long_near_max_ok.out" 2>&1
STATUS_AOT_LONG_OK=$?
set -e

if [[ "$STATUS_AOT_LONG_OK" -ne 0 ]]; then
    echo "Fallo llvm_smoke (aot_long_near_max_ok): no deberia fallar." >&2
    cat "$TMP_DIR/aot_long_near_max_ok.out" >&2
    exit 1
fi

if ! grep -q "runtime bridge LLVM activado" "$TMP_DIR/aot_long_near_max_ok.log"; then
    echo "Fallo llvm_smoke (aot_long_near_max_ok): este caso debe usar runtime bridge para enteros grandes." >&2
    cat "$TMP_DIR/aot_long_near_max_ok.log" >&2
    exit 1
fi

cat > "$TMP_DIR/aot_long_min_ok.clot" <<'PROG'
long value = -9223372036854775808;
print(value);
PROG

AOT_LONG_MIN_EXE="$TMP_DIR/aot_long_min_ok"
"$BIN_PATH" "$TMP_DIR/aot_long_min_ok.clot" --mode compile --emit exe -o "$AOT_LONG_MIN_EXE" --verbose >"$TMP_DIR/aot_long_min_ok.log" 2>&1

set +e
"$AOT_LONG_MIN_EXE" >"$TMP_DIR/aot_long_min_ok.out" 2>&1
STATUS_AOT_LONG_MIN=$?
set -e

if [[ "$STATUS_AOT_LONG_MIN" -ne 0 ]]; then
    echo "Fallo llvm_smoke (aot_long_min_ok): no deberia fallar." >&2
    cat "$TMP_DIR/aot_long_min_ok.out" >&2
    exit 1
fi

if ! grep -q "runtime bridge LLVM activado" "$TMP_DIR/aot_long_min_ok.log"; then
    echo "Fallo llvm_smoke (aot_long_min_ok): este caso debe usar runtime bridge para entero minimo." >&2
    cat "$TMP_DIR/aot_long_min_ok.log" >&2
    exit 1
fi

if ! grep -q "^-9223372036854775808$" "$TMP_DIR/aot_long_min_ok.out"; then
    echo "Fallo llvm_smoke (aot_long_min_ok): salida inesperada." >&2
    cat "$TMP_DIR/aot_long_min_ok.out" >&2
    exit 1
fi

cat > "$TMP_DIR/aot_long_overflow.clot" <<'PROG'
long value = 9223372036854775808;
print(value);
PROG

AOT_LONG_EXE="$TMP_DIR/aot_long_overflow"
"$BIN_PATH" "$TMP_DIR/aot_long_overflow.clot" --mode compile --emit exe -o "$AOT_LONG_EXE" >/dev/null 2>&1

set +e
"$AOT_LONG_EXE" >"$TMP_DIR/aot_long_overflow.out" 2>&1
STATUS_AOT_LONG=$?
set -e

if [[ "$STATUS_AOT_LONG" -eq 0 ]]; then
    echo "Fallo llvm_smoke (aot_long_overflow): se esperaba salida con error." >&2
    exit 1
fi

if ! grep -q "Valor fuera de rango para long." "$TMP_DIR/aot_long_overflow.out"; then
    echo "Fallo llvm_smoke (aot_long_overflow): mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/aot_long_overflow.out" >&2
    exit 1
fi

cat > "$TMP_DIR/aot_byte_overflow.clot" <<'PROG'
byte level = 300;
print(level);
PROG

AOT_BYTE_EXE="$TMP_DIR/aot_byte_overflow"
"$BIN_PATH" "$TMP_DIR/aot_byte_overflow.clot" --mode compile --emit exe -o "$AOT_BYTE_EXE" >/dev/null 2>&1

set +e
"$AOT_BYTE_EXE" >"$TMP_DIR/aot_byte_overflow.out" 2>&1
STATUS_AOT_BYTE=$?
set -e

if [[ "$STATUS_AOT_BYTE" -eq 0 ]]; then
    echo "Fallo llvm_smoke (aot_byte_overflow): se esperaba salida con error." >&2
    exit 1
fi

if ! grep -q "Valor fuera de rango para byte (0-255)." "$TMP_DIR/aot_byte_overflow.out"; then
    echo "Fallo llvm_smoke (aot_byte_overflow): mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/aot_byte_overflow.out" >&2
    exit 1
fi

cat > "$TMP_DIR/full_compile.clot" <<'PROG'
import math;
long max = 10;
byte level = 2;
nums = [1, 2, max + level];
user = {name: "Ada", total: nums[2]};

func inc(&value):
    value += 1;
endfunc

inc(max);
print(sum(max, 5));
print(user.name + " total=" + user.total);
PROG

OUTPUT_EXE="$TMP_DIR/full_compile"
BRIDGE_LOG="$TMP_DIR/full_compile.log"
"$BIN_PATH" "$TMP_DIR/full_compile.clot" --mode compile --emit exe -o "$OUTPUT_EXE" --verbose >"$BRIDGE_LOG" 2>&1

if ! grep -q "runtime bridge LLVM activado" "$BRIDGE_LOG"; then
    echo "Fallo llvm_smoke: este caso debe usar runtime bridge para features completas." >&2
    cat "$BRIDGE_LOG" >&2
    exit 1
fi

EXPECTED=$'16\nAda total=12'
ACTUAL="$($OUTPUT_EXE)"
if [[ "$ACTUAL" != "$EXPECTED" ]]; then
    echo "Fallo llvm_smoke" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL" >&2
    exit 1
fi

mkdir -p "$TMP_DIR/mods"

cat > "$TMP_DIR/mods/helpers.clot" <<'PROG'
func inc_and_get(&value):
    value += 1;
    return value;
endfunc
PROG

cat > "$TMP_DIR/full_bridge.clot" <<'PROG'
import mods.helpers;
long base = 4;
print(inc_and_get(base));
obj = {items: [1, 2, 3]};
obj.items[1] += 8;
print(obj.items[1]);
PROG

FULL_BRIDGE_EXE="$TMP_DIR/full_bridge"
FULL_BRIDGE_LOG="$TMP_DIR/full_bridge.log"
"$BIN_PATH" "$TMP_DIR/full_bridge.clot" --mode compile --emit exe -o "$FULL_BRIDGE_EXE" --verbose >"$FULL_BRIDGE_LOG" 2>&1

if ! grep -q "runtime bridge LLVM activado" "$FULL_BRIDGE_LOG"; then
    echo "Fallo llvm_smoke: full_bridge debe usar runtime bridge." >&2
    cat "$FULL_BRIDGE_LOG" >&2
    exit 1
fi

EXPECTED_BRIDGE=$'5\n10'
ACTUAL_BRIDGE="$($FULL_BRIDGE_EXE)"
if [[ "$ACTUAL_BRIDGE" != "$EXPECTED_BRIDGE" ]]; then
    echo "Fallo llvm_smoke (full_bridge)" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_BRIDGE" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_BRIDGE" >&2
    exit 1
fi

cat > "$TMP_DIR/external_bridge.clot" <<'PROG'
nums = [1, 2, 3];
print(nums[1]);
PROG

EXTERNAL_BRIDGE_EXE="$TMP_DIR/external_bridge"
EXTERNAL_BRIDGE_LOG="$TMP_DIR/external_bridge.log"
"$BIN_PATH" "$TMP_DIR/external_bridge.clot" --mode compile --emit exe -o "$EXTERNAL_BRIDGE_EXE" --runtime-bridge external --verbose >"$EXTERNAL_BRIDGE_LOG" 2>&1

if ! grep -q "runtime bridge externo activado" "$EXTERNAL_BRIDGE_LOG"; then
    echo "Fallo llvm_smoke: external_bridge debe activar runtime bridge externo." >&2
    cat "$EXTERNAL_BRIDGE_LOG" >&2
    exit 1
fi

EXPECTED_EXTERNAL=$'2'
ACTUAL_EXTERNAL="$(PATH="$(dirname "$BIN_PATH"):$PATH" "$EXTERNAL_BRIDGE_EXE")"
if [[ "$ACTUAL_EXTERNAL" != "$EXPECTED_EXTERNAL" ]]; then
    echo "Fallo llvm_smoke (external_bridge)" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_EXTERNAL" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_EXTERNAL" >&2
    exit 1
fi

echo "LLVM smoke tests OK"
