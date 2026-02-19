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
long max = 10;
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

echo "LLVM smoke tests OK"
