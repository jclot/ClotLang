#!/usr/bin/env bash
set -euo pipefail

BIN_PATH="${1:-./build/clot}"

if [[ ! -x "$BIN_PATH" ]]; then
    echo "Binary no encontrado o no ejecutable: $BIN_PATH" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT

cat > "$TMP_DIR/arithmetic.clot" <<'PROG'
a = 2;
a += 5;
print(a);
if(a == 7):
    print("ok");
else:
    print("bad");
endif
PROG

EXPECTED_ONE=$'7\nok'
ACTUAL_ONE="$($BIN_PATH "$TMP_DIR/arithmetic.clot")"
if [[ "$ACTUAL_ONE" != "$EXPECTED_ONE" ]]; then
    echo "Fallo test arithmetic" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_ONE" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_ONE" >&2
    exit 1
fi

cat > "$TMP_DIR/strings.clot" <<'PROG'
name = "Clot";
print("Hola " + name);
PROG

EXPECTED_TWO=$'Hola Clot'
ACTUAL_TWO="$($BIN_PATH "$TMP_DIR/strings.clot")"
if [[ "$ACTUAL_TWO" != "$EXPECTED_TWO" ]]; then
    echo "Fallo test strings" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_TWO" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_TWO" >&2
    exit 1
fi

cat > "$TMP_DIR/migration.clot" <<'PROG'
import math;
a = 5;
b = 7;
print(sum(a, b));
nums = [1, 2, a + b];
user = {name: "Ada", active: true, total: nums[2]};
print(user.name + " total=" + user.total);

func inc(&value):
    value += 1;
endfunc

inc(a);
print(a);
PROG

EXPECTED_THREE=$'12\nAda total=12\n6'
ACTUAL_THREE="$($BIN_PATH "$TMP_DIR/migration.clot")"
if [[ "$ACTUAL_THREE" != "$EXPECTED_THREE" ]]; then
    echo "Fallo test migration" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_THREE" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_THREE" >&2
    exit 1
fi

mkdir -p "$TMP_DIR/mods"

cat > "$TMP_DIR/mods/helpers.clot" <<'PROG'
func inc_and_get(&value):
    value += 1;
    return value;
endfunc
PROG

cat > "$TMP_DIR/modules_return_mutation.clot" <<'PROG'
import mods.helpers;
long base = 7;
print(inc_and_get(base));
print(base);

obj = {items: [1, 2, 3]};
obj.items[1] = 10;
obj.items[1] += 5;
print(obj.items[1]);
PROG

EXPECTED_FOUR=$'8\n8\n15'
ACTUAL_FOUR="$($BIN_PATH "$TMP_DIR/modules_return_mutation.clot")"
if [[ "$ACTUAL_FOUR" != "$EXPECTED_FOUR" ]]; then
    echo "Fallo test modules_return_mutation" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_FOUR" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_FOUR" >&2
    exit 1
fi

cat > "$TMP_DIR/i18n_en_error.clot" <<'PROG'
print(unknown_var);
PROG

set +e
CLOT_LANG=en "$BIN_PATH" "$TMP_DIR/i18n_en_error.clot" >"$TMP_DIR/i18n_en.out" 2>"$TMP_DIR/i18n_en.err"
STATUS_EN=$?
set -e

if [[ "$STATUS_EN" -eq 0 ]]; then
    echo "Fallo test i18n_en_error: se esperaba fallo de ejecucion." >&2
    exit 1
fi

if ! grep -q "Runtime error: Undefined variable: unknown_var" "$TMP_DIR/i18n_en.err"; then
    echo "Fallo test i18n_en_error: mensaje esperado en ingles no encontrado." >&2
    echo "stderr actual:" >&2
    cat "$TMP_DIR/i18n_en.err" >&2
    exit 1
fi

cat > "$TMP_DIR/long_near_max_ok.clot" <<'PROG'
long value = 9223372036854775800;
print(value);
PROG

EXPECTED_LONG_NEAR_MAX=$'9223372036854775800'
ACTUAL_LONG_NEAR_MAX="$($BIN_PATH "$TMP_DIR/long_near_max_ok.clot")"
if [[ "$ACTUAL_LONG_NEAR_MAX" != "$EXPECTED_LONG_NEAR_MAX" ]]; then
    echo "Fallo test long_near_max_ok" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_LONG_NEAR_MAX" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_LONG_NEAR_MAX" >&2
    exit 1
fi

cat > "$TMP_DIR/long_near_max_mutation_ok.clot" <<'PROG'
long value = 9223372036854775800;
value += 0;
print(value);
PROG

EXPECTED_LONG_MUT=$'9223372036854775800'
ACTUAL_LONG_MUT="$($BIN_PATH "$TMP_DIR/long_near_max_mutation_ok.clot")"
if [[ "$ACTUAL_LONG_MUT" != "$EXPECTED_LONG_MUT" ]]; then
    echo "Fallo test long_near_max_mutation_ok" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_LONG_MUT" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_LONG_MUT" >&2
    exit 1
fi

cat > "$TMP_DIR/long_overflow.clot" <<'PROG'
long value = 9223372036854775808;
print(value);
PROG

set +e
"$BIN_PATH" "$TMP_DIR/long_overflow.clot" >"$TMP_DIR/long_overflow.out" 2>"$TMP_DIR/long_overflow.err"
STATUS_LONG=$?
set -e

if [[ "$STATUS_LONG" -eq 0 ]]; then
    echo "Fallo test long_overflow: se esperaba error de rango." >&2
    exit 1
fi

if ! grep -q "Valor fuera de rango para long." "$TMP_DIR/long_overflow.err"; then
    echo "Fallo test long_overflow: mensaje de rango no encontrado." >&2
    cat "$TMP_DIR/long_overflow.err" >&2
    exit 1
fi

cat > "$TMP_DIR/list_non_integer_index.clot" <<'PROG'
nums = [1, 2, 3];
print(nums[1.5]);
PROG

set +e
"$BIN_PATH" "$TMP_DIR/list_non_integer_index.clot" >"$TMP_DIR/list_non_integer_index.out" 2>"$TMP_DIR/list_non_integer_index.err"
STATUS_INDEX=$?
set -e

if [[ "$STATUS_INDEX" -eq 0 ]]; then
    echo "Fallo test list_non_integer_index: se esperaba error de indice." >&2
    exit 1
fi

if ! grep -q "El indice de lista debe ser un entero finito." "$TMP_DIR/list_non_integer_index.err"; then
    echo "Fallo test list_non_integer_index: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/list_non_integer_index.err" >&2
    exit 1
fi

echo "Smoke tests OK"
