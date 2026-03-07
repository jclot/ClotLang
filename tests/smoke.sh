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
println(a);
if(a == 7):
    println("ok");
else:
    println("bad");
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
println(sum(a, b));
nums = [1, 2, a + b];
user = {name: "Ada", active: true, total: nums[2]};
println(user.name + " total=" + user.total);

func inc(&value):
    value += 1;
endfunc

inc(a);
println(a);
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

mkdir -p "$TMP_DIR/clot/core/helpers"

cat > "$TMP_DIR/clot/core/helpers/helpers.clot" <<'PROG'
func inc_and_get(&value):
    value += 1;
    return value;
endfunc
PROG

cat > "$TMP_DIR/modules_return_mutation.clot" <<'PROG'
import clot.core.helpers;
long base = 7;
println(inc_and_get(base));
println(base);

obj = {items: [1, 2, 3]};
obj.items[1] = 10;
obj.items[1] += 5;
println(obj.items[1]);
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

mkdir -p "$TMP_DIR/clot/science/utils" "$TMP_DIR/clot/core/helpers"

cat > "$TMP_DIR/clot/science/utils/utils.clot" <<'PROG'
func root_tag():
    return "clot-science";
endfunc

func util_value():
    return 41;
endfunc
PROG

cat > "$TMP_DIR/clot/core/helpers/helpers.clot" <<'PROG'
import clot.science.utils;

func helper_tag():
    return "clot-core";
endfunc

func helper_plus_one():
    return util_value() + 1;
endfunc
PROG

cat > "$TMP_DIR/package_roots.clot" <<'PROG'
import clot.science.utils;
import clot.core.helpers;
println(root_tag());
println(helper_tag());
PROG

EXPECTED_PACKAGE_ROOTS=$'clot-science\nclot-core'
ACTUAL_PACKAGE_ROOTS="$($BIN_PATH "$TMP_DIR/package_roots.clot")"
if [[ "$ACTUAL_PACKAGE_ROOTS" != "$EXPECTED_PACKAGE_ROOTS" ]]; then
    echo "Fallo test package_roots" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_PACKAGE_ROOTS" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_PACKAGE_ROOTS" >&2
    exit 1
fi

cat > "$TMP_DIR/package_aliases.clot" <<'PROG'
import modules.utils;
import mods.helpers;
println(root_tag());
println(helper_tag());
PROG

ACTUAL_PACKAGE_ALIASES="$($BIN_PATH "$TMP_DIR/package_aliases.clot")"
if [[ "$ACTUAL_PACKAGE_ALIASES" != "$EXPECTED_PACKAGE_ROOTS" ]]; then
    echo "Fallo test package_aliases" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_PACKAGE_ROOTS" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_PACKAGE_ALIASES" >&2
    exit 1
fi

cat > "$TMP_DIR/package_nested.clot" <<'PROG'
import clot.core.helpers;
println(helper_plus_one());
PROG

EXPECTED_PACKAGE_NESTED=$'42'
ACTUAL_PACKAGE_NESTED="$($BIN_PATH "$TMP_DIR/package_nested.clot")"
if [[ "$ACTUAL_PACKAGE_NESTED" != "$EXPECTED_PACKAGE_NESTED" ]]; then
    echo "Fallo test package_nested" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_PACKAGE_NESTED" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_PACKAGE_NESTED" >&2
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

cat > "$TMP_DIR/try_catch.clot" <<'PROG'
try:
    nums = [1, 2];
    println(nums[5]);
catch(err):
    println("captured");
    println(err);
endtry
println("after");
PROG

EXPECTED_TRY_CATCH=$'captured\nIndice fuera de rango en lista.\nafter'
ACTUAL_TRY_CATCH="$($BIN_PATH "$TMP_DIR/try_catch.clot")"
if [[ "$ACTUAL_TRY_CATCH" != "$EXPECTED_TRY_CATCH" ]]; then
    echo "Fallo test try_catch" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_TRY_CATCH" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_TRY_CATCH" >&2
    exit 1
fi

EXPECTED_TRY_CATCH_EN=$'captured\nList index out of bounds.\nafter'
ACTUAL_TRY_CATCH_EN="$($BIN_PATH "$TMP_DIR/try_catch.clot" --lang en)"
if [[ "$ACTUAL_TRY_CATCH_EN" != "$EXPECTED_TRY_CATCH_EN" ]]; then
    echo "Fallo test try_catch_en" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_TRY_CATCH_EN" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_TRY_CATCH_EN" >&2
    exit 1
fi

IO_FILE="$TMP_DIR/io_data.txt"
cat > "$TMP_DIR/async_io.clot" <<PROG
write_file("$IO_FILE", "hola");
task = async_read_file("$IO_FILE");
println(await(task));
append_file("$IO_FILE", "!");
println(read_file("$IO_FILE"));
println(file_exists("$IO_FILE"));
PROG

EXPECTED_ASYNC_IO=$'hola\nhola!\ntrue'
ACTUAL_ASYNC_IO="$($BIN_PATH "$TMP_DIR/async_io.clot")"
if [[ "$ACTUAL_ASYNC_IO" != "$EXPECTED_ASYNC_IO" ]]; then
    echo "Fallo test async_io" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_ASYNC_IO" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_ASYNC_IO" >&2
    exit 1
fi

cat > "$TMP_DIR/analyze_fail.clot" <<'PROG'
long x = "texto";
print(y);
PROG

cat > "$TMP_DIR/print_no_newline.clot" <<'PROG'
print("A");
print("B");
println("C");
println();
println("D");
PROG

EXPECTED_PRINT_NO_NL=$'ABC\n\nD'
ACTUAL_PRINT_NO_NL="$($BIN_PATH "$TMP_DIR/print_no_newline.clot")"
if [[ "$ACTUAL_PRINT_NO_NL" != "$EXPECTED_PRINT_NO_NL" ]]; then
    echo "Fallo test print_no_newline" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_PRINT_NO_NL" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_PRINT_NO_NL" >&2
    exit 1
fi

cat > "$TMP_DIR/printf_formats.clot" <<'PROG'
printf("%i|%u|%x|%X|%f|%c|%s|%%", -7, 7, 255, 255, 2.5, "Z", "hola");
println();
PROG

EXPECTED_PRINTF=$'-7|7|ff|FF|2.500000|Z|hola|%'
ACTUAL_PRINTF="$($BIN_PATH "$TMP_DIR/printf_formats.clot")"
if [[ "$ACTUAL_PRINTF" != "$EXPECTED_PRINTF" ]]; then
    echo "Fallo test printf_formats" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_PRINTF" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_PRINTF" >&2
    exit 1
fi

cat > "$TMP_DIR/bigint_math_core.clot" <<'PROG'
import math;
a = 9223372036854775808;
a += 5;
println(a);

double ratio = 2.5;
println(ratio + 0.5);
println(type(a));
println(cast("42", "int"));
println(sqrt(16.0));
println(pow(2, 10));
println(gcd(84, 30));
println(lcm(21, 6));
assert(a > 0);
println("ok");
PROG

EXPECTED_BIGINT_MATH_CORE=$'9223372036854775813\n3\nint\n42\n4\n1024\n6\n42\nok'
ACTUAL_BIGINT_MATH_CORE="$($BIN_PATH "$TMP_DIR/bigint_math_core.clot")"
if [[ "$ACTUAL_BIGINT_MATH_CORE" != "$EXPECTED_BIGINT_MATH_CORE" ]]; then
    echo "Fallo test bigint_math_core" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_BIGINT_MATH_CORE" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_BIGINT_MATH_CORE" >&2
    exit 1
fi

set +e
"$BIN_PATH" "$TMP_DIR/analyze_fail.clot" --mode analyze >"$TMP_DIR/analyze_fail.out" 2>"$TMP_DIR/analyze_fail.err"
STATUS_ANALYZE=$?
set -e

if [[ "$STATUS_ANALYZE" -eq 0 ]]; then
    echo "Fallo test analyze_fail: se esperaba error en analisis estatico." >&2
    exit 1
fi

if ! grep -q "Analisis estatico" "$TMP_DIR/analyze_fail.err"; then
    echo "Fallo test analyze_fail: no se detectaron diagnosticos de analisis." >&2
    cat "$TMP_DIR/analyze_fail.err" >&2
    exit 1
fi

cat > "$TMP_DIR/new_types.clot" <<'PROG'
func id(x):
    return x;
endfunc

enum Estado { ACTIVO, INACTIVO };
char c = 'Z';
float f = 1.5;
decimal d = cast("12.34", "decimal");
tuple t = tuple(1, 2, c);
set s = set(1, 2, 2, 3);
map m = map("a", 1, 2, "dos");
function fn = id;

println(type(Estado));
println(Estado.ACTIVO);
println(type(c));
println(type(f));
println(type(d));
println(type(t));
println(type(s));
println(type(m));
println(type(fn));
println(type(null));
println(m[2]);
println(fn(10));
PROG

EXPECTED_NEW_TYPES=$'object\n0\nchar\nfloat\ndecimal\ntuple\nset\nmap\nfunction\nnull\ndos\n10'
ACTUAL_NEW_TYPES="$($BIN_PATH "$TMP_DIR/new_types.clot")"
if [[ "$ACTUAL_NEW_TYPES" != "$EXPECTED_NEW_TYPES" ]]; then
    echo "Fallo test new_types" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_NEW_TYPES" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_NEW_TYPES" >&2
    exit 1
fi

cat > "$TMP_DIR/tuple_mutation_error.clot" <<'PROG'
tuple t = tuple(1, 2);
t[0] = 9;
PROG

set +e
"$BIN_PATH" "$TMP_DIR/tuple_mutation_error.clot" --lang en >"$TMP_DIR/tuple_mutation_error.out" 2>"$TMP_DIR/tuple_mutation_error.err"
STATUS_TUPLE_MUT=$?
set -e

if [[ "$STATUS_TUPLE_MUT" -eq 0 ]]; then
    echo "Fallo test tuple_mutation_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "Runtime error: Cannot mutate a tuple with \\[\\]." "$TMP_DIR/tuple_mutation_error.err"; then
    echo "Fallo test tuple_mutation_error: mensaje esperado en ingles no encontrado." >&2
    cat "$TMP_DIR/tuple_mutation_error.err" >&2
    exit 1
fi

cat > "$TMP_DIR/function_no_params_and_enum_multiline.clot" <<'PROG'
func prenderTV():
    println("TV Encendida");
endfunc

enum Estado {
    ACTIVO,
    INACTIVO
};

prenderTV();
println(enum_name(Estado, Estado.INACTIVO));
println(enum_value(Estado, "ACTIVO"));
PROG

EXPECTED_FN_ENUM=$'TV Encendida\nINACTIVO\n0'
ACTUAL_FN_ENUM="$($BIN_PATH "$TMP_DIR/function_no_params_and_enum_multiline.clot")"
if [[ "$ACTUAL_FN_ENUM" != "$EXPECTED_FN_ENUM" ]]; then
    echo "Fallo test function_no_params_and_enum_multiline" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_FN_ENUM" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_FN_ENUM" >&2
    exit 1
fi

cat > "$TMP_DIR/function_ref_with_parens.clot" <<'PROG'
func prenderTV():
    println("TV Encendida");
endfunc

function control = prenderTV();
control();
PROG

EXPECTED_FUNCTION_REF_PARENS=$'TV Encendida'
ACTUAL_FUNCTION_REF_PARENS="$($BIN_PATH "$TMP_DIR/function_ref_with_parens.clot")"
if [[ "$ACTUAL_FUNCTION_REF_PARENS" != "$EXPECTED_FUNCTION_REF_PARENS" ]]; then
    echo "Fallo test function_ref_with_parens" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_FUNCTION_REF_PARENS" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_FUNCTION_REF_PARENS" >&2
    exit 1
fi

cat > "$TMP_DIR/function_implicit_null.clot" <<'PROG'
func prenderTV():
    println("TV Encendida");
endfunc

println(prenderTV());
PROG

EXPECTED_FUNCTION_IMPLICIT_NULL=$'TV Encendida\nnull'
ACTUAL_FUNCTION_IMPLICIT_NULL="$($BIN_PATH "$TMP_DIR/function_implicit_null.clot")"
if [[ "$ACTUAL_FUNCTION_IMPLICIT_NULL" != "$EXPECTED_FUNCTION_IMPLICIT_NULL" ]]; then
    echo "Fallo test function_implicit_null" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_FUNCTION_IMPLICIT_NULL" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_FUNCTION_IMPLICIT_NULL" >&2
    exit 1
fi

cat > "$TMP_DIR/function_type_hints_ok.clot" <<'PROG'
func string greet(name: string):
    return "Hola " + name;
endfunc

func float average(a: float, b: float):
    return (a + b) / 2;
endfunc

println(greet("Ada"));
println(type(average(5, 7)));
println(average(5, 7));
PROG

EXPECTED_FUNCTION_TYPE_HINTS_OK=$'Hola Ada\nfloat\n6'
ACTUAL_FUNCTION_TYPE_HINTS_OK="$($BIN_PATH "$TMP_DIR/function_type_hints_ok.clot")"
if [[ "$ACTUAL_FUNCTION_TYPE_HINTS_OK" != "$EXPECTED_FUNCTION_TYPE_HINTS_OK" ]]; then
    echo "Fallo test function_type_hints_ok" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_FUNCTION_TYPE_HINTS_OK" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_FUNCTION_TYPE_HINTS_OK" >&2
    exit 1
fi

cat > "$TMP_DIR/function_type_hints_missing_return.clot" <<'PROG'
func int no_return():
    x = 1;
endfunc

no_return();
PROG

set +e
"$BIN_PATH" "$TMP_DIR/function_type_hints_missing_return.clot" >"$TMP_DIR/function_type_hints_missing_return.out" 2>"$TMP_DIR/function_type_hints_missing_return.err"
STATUS_FUNCTION_TYPE_MISSING_RETURN=$?
set -e

if [[ "$STATUS_FUNCTION_TYPE_MISSING_RETURN" -eq 0 ]]; then
    echo "Fallo test function_type_hints_missing_return: se esperaba error." >&2
    exit 1
fi

if ! grep -q "debe retornar un valor de tipo 'int'" "$TMP_DIR/function_type_hints_missing_return.err"; then
    echo "Fallo test function_type_hints_missing_return: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/function_type_hints_missing_return.err" >&2
    exit 1
fi

cat > "$TMP_DIR/function_type_hints_bad_arg.clot" <<'PROG'
func string greet(name: string):
    return name;
endfunc

println(greet(10));
PROG

set +e
"$BIN_PATH" "$TMP_DIR/function_type_hints_bad_arg.clot" >"$TMP_DIR/function_type_hints_bad_arg.out" 2>"$TMP_DIR/function_type_hints_bad_arg.err"
STATUS_FUNCTION_TYPE_BAD_ARG=$?
set -e

if [[ "$STATUS_FUNCTION_TYPE_BAD_ARG" -eq 0 ]]; then
    echo "Fallo test function_type_hints_bad_arg: se esperaba error." >&2
    exit 1
fi

if ! grep -q "Argumento 'name' no coincide con type hint 'string'" "$TMP_DIR/function_type_hints_bad_arg.err"; then
    echo "Fallo test function_type_hints_bad_arg: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/function_type_hints_bad_arg.err" >&2
    exit 1
fi

cat > "$TMP_DIR/function_defaults_ok.clot" <<'PROG'
func float free_fall_height(h0: float, v0: float, t: float, g: float = 9.81):
    return h0 + v0 * t - (g * t * t) / 2;
endfunc

println(free_fall_height(100.0, 0.0, 2.0));
println(free_fall_height(100.0, 0.0, 2.0, 10.0));
println(type(free_fall_height(100.0, 0.0, 2.0)));
PROG

EXPECTED_FUNCTION_DEFAULTS_OK=$'80.38\n80\nfloat'
ACTUAL_FUNCTION_DEFAULTS_OK="$($BIN_PATH "$TMP_DIR/function_defaults_ok.clot")"
if [[ "$ACTUAL_FUNCTION_DEFAULTS_OK" != "$EXPECTED_FUNCTION_DEFAULTS_OK" ]]; then
    echo "Fallo test function_defaults_ok" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_FUNCTION_DEFAULTS_OK" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_FUNCTION_DEFAULTS_OK" >&2
    exit 1
fi

cat > "$TMP_DIR/function_defaults_dynamic_ok.clot" <<'PROG'
func greet(name, prefix = "Hola "):
    return prefix + name;
endfunc

println(greet("Ada"));
println(greet("Ada", "Hey "));
PROG

EXPECTED_FUNCTION_DEFAULTS_DYNAMIC_OK=$'Hola Ada\nHey Ada'
ACTUAL_FUNCTION_DEFAULTS_DYNAMIC_OK="$($BIN_PATH "$TMP_DIR/function_defaults_dynamic_ok.clot")"
if [[ "$ACTUAL_FUNCTION_DEFAULTS_DYNAMIC_OK" != "$EXPECTED_FUNCTION_DEFAULTS_DYNAMIC_OK" ]]; then
    echo "Fallo test function_defaults_dynamic_ok" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_FUNCTION_DEFAULTS_DYNAMIC_OK" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_FUNCTION_DEFAULTS_DYNAMIC_OK" >&2
    exit 1
fi

cat > "$TMP_DIR/function_defaults_order_parse_error.clot" <<'PROG'
func bad(a = 1, b):
    return a + b;
endfunc

bad(1, 2);
PROG

set +e
"$BIN_PATH" "$TMP_DIR/function_defaults_order_parse_error.clot" >"$TMP_DIR/function_defaults_order_parse_error.out" 2>"$TMP_DIR/function_defaults_order_parse_error.err"
STATUS_FUNCTION_DEFAULTS_ORDER=$?
set -e

if [[ "$STATUS_FUNCTION_DEFAULTS_ORDER" -eq 0 ]]; then
    echo "Fallo test function_defaults_order_parse_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "no pueden ir despues" "$TMP_DIR/function_defaults_order_parse_error.err"; then
    echo "Fallo test function_defaults_order_parse_error: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/function_defaults_order_parse_error.err" >&2
    exit 1
fi

cat > "$TMP_DIR/function_defaults_ref_parse_error.clot" <<'PROG'
func bad(&x = 1):
    return x;
endfunc

v = 1;
bad(v);
PROG

set +e
"$BIN_PATH" "$TMP_DIR/function_defaults_ref_parse_error.clot" >"$TMP_DIR/function_defaults_ref_parse_error.out" 2>"$TMP_DIR/function_defaults_ref_parse_error.err"
STATUS_FUNCTION_DEFAULTS_REF=$?
set -e

if [[ "$STATUS_FUNCTION_DEFAULTS_REF" -eq 0 ]]; then
    echo "Fallo test function_defaults_ref_parse_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "por referencia no aceptan valor por defecto" "$TMP_DIR/function_defaults_ref_parse_error.err"; then
    echo "Fallo test function_defaults_ref_parse_error: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/function_defaults_ref_parse_error.err" >&2
    exit 1
fi

cat > "$TMP_DIR/oop_mvp_ok.clot" <<'PROG'
interface Named:
    func string name();
endinterface

class Animal:
    private string _kind = "unknown";

    constructor(kind: string):
        this._kind = kind;
    endconstructor

    public func string kind():
        return this._kind;
    endfunc
endclass

class Dog extends Animal implements Named:
    private string _name;
    public static int total = 0;
    public readonly string id;

    constructor(name: string):
        super("dog");
        this._name = name;
        this.id = "D-" + (Dog.total + 1);
        Dog.total += 1;
    endconstructor

    public override func string kind():
        return super.kind() + ":" + this._name;
    endfunc

    public func string name():
        return this._name;
    endfunc

    public get alias():
        return this._name;
    endget

    public set alias(value: string):
        this._name = value;
    endset
endclass

d = Dog("Neo");
println(d.kind());
println(d.name());
println(d.alias);
d.alias = "Trinity";
println(d.alias);
println(Dog.total);
PROG

EXPECTED_OOP_MVP_OK=$'dog:Neo\nNeo\nNeo\nTrinity\n1'
ACTUAL_OOP_MVP_OK="$($BIN_PATH "$TMP_DIR/oop_mvp_ok.clot")"
if [[ "$ACTUAL_OOP_MVP_OK" != "$EXPECTED_OOP_MVP_OK" ]]; then
    echo "Fallo test oop_mvp_ok" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_OOP_MVP_OK" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_OOP_MVP_OK" >&2
    exit 1
fi

cat > "$TMP_DIR/oop_private_error.clot" <<'PROG'
class Secret:
    private string token = "x";
endclass

s = Secret();
println(s.token);
PROG

set +e
"$BIN_PATH" "$TMP_DIR/oop_private_error.clot" >"$TMP_DIR/oop_private_error.out" 2>"$TMP_DIR/oop_private_error.err"
STATUS_OOP_PRIVATE=$?
set -e

if [[ "$STATUS_OOP_PRIVATE" -eq 0 ]]; then
    echo "Fallo test oop_private_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "Campo no accesible por visibilidad: Secret.token" "$TMP_DIR/oop_private_error.err"; then
    echo "Fallo test oop_private_error: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/oop_private_error.err" >&2
    exit 1
fi

set +e
"$BIN_PATH" "$TMP_DIR/oop_private_error.clot" --lang en >"$TMP_DIR/oop_private_error_en.out" 2>"$TMP_DIR/oop_private_error_en.err"
STATUS_OOP_PRIVATE_EN=$?
set -e

if [[ "$STATUS_OOP_PRIVATE_EN" -eq 0 ]]; then
    echo "Fallo test oop_private_error_en: se esperaba error." >&2
    exit 1
fi

if ! grep -q "Runtime error: Field not accessible due to visibility: Secret.token" "$TMP_DIR/oop_private_error_en.err"; then
    echo "Fallo test oop_private_error_en: mensaje esperado en ingles no encontrado." >&2
    cat "$TMP_DIR/oop_private_error_en.err" >&2
    exit 1
fi

cat > "$TMP_DIR/oop_readonly_error.clot" <<'PROG'
class Box:
    public readonly int value = 10;
endclass

b = Box();
b.value = 20;
PROG

set +e
"$BIN_PATH" "$TMP_DIR/oop_readonly_error.clot" >"$TMP_DIR/oop_readonly_error.out" 2>"$TMP_DIR/oop_readonly_error.err"
STATUS_OOP_READONLY=$?
set -e

if [[ "$STATUS_OOP_READONLY" -eq 0 ]]; then
    echo "Fallo test oop_readonly_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "No se puede modificar campo readonly: Box.value" "$TMP_DIR/oop_readonly_error.err"; then
    echo "Fallo test oop_readonly_error: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/oop_readonly_error.err" >&2
    exit 1
fi

cat > "$TMP_DIR/oop_override_required_error.clot" <<'PROG'
class A:
    public func int f():
        return 1;
    endfunc
endclass

class B extends A:
    public func int f():
        return 2;
    endfunc
endclass
PROG

set +e
"$BIN_PATH" "$TMP_DIR/oop_override_required_error.clot" >"$TMP_DIR/oop_override_required_error.out" 2>"$TMP_DIR/oop_override_required_error.err"
STATUS_OOP_OVERRIDE=$?
set -e

if [[ "$STATUS_OOP_OVERRIDE" -eq 0 ]]; then
    echo "Fallo test oop_override_required_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "Metodo sobrescribe base y requiere 'override': B.f" "$TMP_DIR/oop_override_required_error.err"; then
    echo "Fallo test oop_override_required_error: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/oop_override_required_error.err" >&2
    exit 1
fi

cat > "$TMP_DIR/oop_interface_missing_error.clot" <<'PROG'
interface Pinger:
    func ping();
endinterface

class Bot implements Pinger:
endclass
PROG

set +e
"$BIN_PATH" "$TMP_DIR/oop_interface_missing_error.clot" >"$TMP_DIR/oop_interface_missing_error.out" 2>"$TMP_DIR/oop_interface_missing_error.err"
STATUS_OOP_INTERFACE=$?
set -e

if [[ "$STATUS_OOP_INTERFACE" -eq 0 ]]; then
    echo "Fallo test oop_interface_missing_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "no implementa metodo 'ping'" "$TMP_DIR/oop_interface_missing_error.err"; then
    echo "Fallo test oop_interface_missing_error: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/oop_interface_missing_error.err" >&2
    exit 1
fi

cat > "$TMP_DIR/oop_super_first_error.clot" <<'PROG'
class Base:
    constructor():
    endconstructor
endclass

class Child extends Base:
    constructor():
        x = 1;
        super();
    endconstructor
endclass

c = Child();
println(c);
PROG

set +e
"$BIN_PATH" "$TMP_DIR/oop_super_first_error.clot" >"$TMP_DIR/oop_super_first_error.out" 2>"$TMP_DIR/oop_super_first_error.err"
STATUS_OOP_SUPER=$?
set -e

if [[ "$STATUS_OOP_SUPER" -eq 0 ]]; then
    echo "Fallo test oop_super_first_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "debe invocar super(...) como primera sentencia" "$TMP_DIR/oop_super_first_error.err"; then
    echo "Fallo test oop_super_first_error: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/oop_super_first_error.err" >&2
    exit 1
fi

echo "Smoke tests OK"
