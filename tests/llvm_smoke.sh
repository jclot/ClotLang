#!/usr/bin/env bash
set -euo pipefail

BIN_PATH="${1:-./build/wsl-release/clot}"

if [[ ! -x "$BIN_PATH" ]]; then
    echo "Binary no encontrado o no ejecutable: $BIN_PATH" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

cat > "$TMP_DIR/aot_double_math.clot" <<'PROG'
import math;
double a = 16.0;
double b = sqrt(a);
double c = sum(b, 2.0);
println(c);
println(log(100.0));
println(factorial(5.0));
PROG

AOT_EXE="$TMP_DIR/aot_double_math"
AOT_LOG="$TMP_DIR/aot_double_math.log"
"$BIN_PATH" "$TMP_DIR/aot_double_math.clot" --mode compile --emit exe -o "$AOT_EXE" --verbose >"$AOT_LOG" 2>&1

if grep -q "runtime bridge LLVM activado" "$AOT_LOG"; then
    echo "Fallo llvm_smoke: aot_double_math deberia compilar AOT puro." >&2
    cat "$AOT_LOG" >&2
    exit 1
fi

EXPECTED_AOT=$'6\n2\n120'
ACTUAL_AOT="$($AOT_EXE)"
if [[ "$ACTUAL_AOT" != "$EXPECTED_AOT" ]]; then
    echo "Fallo llvm_smoke (aot_double_math)" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_AOT" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_AOT" >&2
    exit 1
fi

cat > "$TMP_DIR/int_default_bridge.clot" <<'PROG'
import math;
x = 10;
println(sum(x, 2));
PROG

INT_BRIDGE_EXE="$TMP_DIR/int_default_bridge"
INT_BRIDGE_LOG="$TMP_DIR/int_default_bridge.log"
"$BIN_PATH" "$TMP_DIR/int_default_bridge.clot" --mode compile --emit exe -o "$INT_BRIDGE_EXE" --verbose >"$INT_BRIDGE_LOG" 2>&1

if ! grep -q "runtime bridge LLVM activado" "$INT_BRIDGE_LOG"; then
    echo "Fallo llvm_smoke: int_default_bridge debe activar runtime bridge." >&2
    cat "$INT_BRIDGE_LOG" >&2
    exit 1
fi

EXPECTED_INT_BRIDGE=$'12'
ACTUAL_INT_BRIDGE="$($INT_BRIDGE_EXE)"
if [[ "$ACTUAL_INT_BRIDGE" != "$EXPECTED_INT_BRIDGE" ]]; then
    echo "Fallo llvm_smoke (int_default_bridge)" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_INT_BRIDGE" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_INT_BRIDGE" >&2
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
