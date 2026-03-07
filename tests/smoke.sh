#!/usr/bin/env bash
set -euo pipefail

BIN_PATH="${1:-./build/clot}"

if [[ ! -x "$BIN_PATH" ]]; then
    echo "Binary no encontrado o no ejecutable: $BIN_PATH" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TMP_DIR"' EXIT
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

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

mkdir -p "$TMP_DIR/clot/science"
cp -r "$REPO_ROOT/clot/science/physics" "$TMP_DIR/clot/science/physics"

cat > "$TMP_DIR/physics_electromagnetism.clot" <<'PROG'
import clot.science.physics;
println(ohms_current(10, 2));
println(resistor_parallel(6, 3));
println(capacitor_series(3, 6));
println(wave_speed(4, 5));

import clot.science.physics.electromagnetism;
println(electric_flux(1));
PROG

EXPECTED_PHYSICS_ELECTROMAGNETISM=$'5\n2\n2\n20\n112940906737.302'
ACTUAL_PHYSICS_ELECTROMAGNETISM="$($BIN_PATH "$TMP_DIR/physics_electromagnetism.clot")"
if [[ "$ACTUAL_PHYSICS_ELECTROMAGNETISM" != "$EXPECTED_PHYSICS_ELECTROMAGNETISM" ]]; then
    echo "Fallo test physics_electromagnetism" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_PHYSICS_ELECTROMAGNETISM" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_PHYSICS_ELECTROMAGNETISM" >&2
    exit 1
fi

mkdir -p "$TMP_DIR/project_root/scripts" "$TMP_DIR/project_root/apps/deep/nested"

cat > "$TMP_DIR/project_root/scripts/math.clot" <<'PROG'
func add(a, b):
    return a + b;
endfunc

class Box:
    public int value = 0;

    constructor(v):
        this.value = v;
    endconstructor
endclass
PROG

cat > "$TMP_DIR/project_root/apps/deep/nested/import_alias_from_root.clot" <<'PROG'
import scripts.math as math;
from scripts.math import add;
from scripts.math import Box as Caja;

println(add(2, 3));
println(math.add(4, 5));

box_one = Caja(7);
println(box_one.value);

box_two = math.Box(9);
println(box_two.value);
PROG

EXPECTED_IMPORT_ALIAS_FROM_ROOT=$'5\n9\n7\n9'
ACTUAL_IMPORT_ALIAS_FROM_ROOT="$($BIN_PATH "$TMP_DIR/project_root/apps/deep/nested/import_alias_from_root.clot")"
if [[ "$ACTUAL_IMPORT_ALIAS_FROM_ROOT" != "$EXPECTED_IMPORT_ALIAS_FROM_ROOT" ]]; then
    echo "Fallo test import_alias_from_root" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_IMPORT_ALIAS_FROM_ROOT" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_IMPORT_ALIAS_FROM_ROOT" >&2
    exit 1
fi

cat > "$TMP_DIR/project_root/scripts/cache.clot" <<'PROG'
hits = 0;
hits += 1;

func hits_count():
    return hits;
endfunc
PROG

cat > "$TMP_DIR/project_root/apps/deep/nested/import_cache_reuse.clot" <<'PROG'
import scripts.cache;
import scripts.cache as cache_mod;
from scripts.cache import hits_count;

println(hits_count());
println(cache_mod.hits_count());
PROG

EXPECTED_IMPORT_CACHE_REUSE=$'1\n1'
ACTUAL_IMPORT_CACHE_REUSE="$($BIN_PATH "$TMP_DIR/project_root/apps/deep/nested/import_cache_reuse.clot")"
if [[ "$ACTUAL_IMPORT_CACHE_REUSE" != "$EXPECTED_IMPORT_CACHE_REUSE" ]]; then
    echo "Fallo test import_cache_reuse" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_IMPORT_CACHE_REUSE" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_IMPORT_CACHE_REUSE" >&2
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

cat > "$TMP_DIR/uncaught_throw.clot" <<'PROG'
throw("fatal");
PROG

set +e
"$BIN_PATH" "$TMP_DIR/uncaught_throw.clot" >"$TMP_DIR/uncaught_throw.out" 2>"$TMP_DIR/uncaught_throw.err"
STATUS_UNCAUGHT_THROW=$?
set -e

if [[ "$STATUS_UNCAUGHT_THROW" -eq 0 ]]; then
    echo "Fallo test uncaught_throw: se esperaba fallo de ejecucion." >&2
    exit 1
fi

if ! grep -q "Excepcion no capturada: RuntimeError: fatal" "$TMP_DIR/uncaught_throw.err"; then
    echo "Fallo test uncaught_throw: mensaje esperado en espanol no encontrado." >&2
    echo "stderr actual:" >&2
    cat "$TMP_DIR/uncaught_throw.err" >&2
    exit 1
fi

set +e
CLOT_LANG=en "$BIN_PATH" "$TMP_DIR/uncaught_throw.clot" >"$TMP_DIR/uncaught_throw_en.out" 2>"$TMP_DIR/uncaught_throw_en.err"
STATUS_UNCAUGHT_THROW_EN=$?
set -e

if [[ "$STATUS_UNCAUGHT_THROW_EN" -eq 0 ]]; then
    echo "Fallo test uncaught_throw_en: se esperaba fallo de ejecucion." >&2
    exit 1
fi

if ! grep -q "Unhandled Exception: RuntimeError: fatal" "$TMP_DIR/uncaught_throw_en.err"; then
    echo "Fallo test uncaught_throw_en: mensaje esperado en ingles no encontrado." >&2
    echo "stderr actual:" >&2
    cat "$TMP_DIR/uncaught_throw_en.err" >&2
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

cat > "$TMP_DIR/throw_builtin_string.clot" <<'PROG'
try:
    throw("boom");
catch(err):
    println("captured");
    println(err);
endtry
PROG

EXPECTED_THROW_BUILTIN=$'captured\nboom'
ACTUAL_THROW_BUILTIN="$($BIN_PATH "$TMP_DIR/throw_builtin_string.clot")"
if [[ "$ACTUAL_THROW_BUILTIN" != "$EXPECTED_THROW_BUILTIN" ]]; then
    echo "Fallo test throw_builtin_string" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_THROW_BUILTIN" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_THROW_BUILTIN" >&2
    exit 1
fi

cat > "$TMP_DIR/typed_catch_runtime.clot" <<'PROG'
try:
    nums = [1, 2];
    println(nums[8]);
catch(IndexError err):
    println("typed");
    println(err);
endtry
PROG

EXPECTED_TYPED_CATCH_RUNTIME=$'typed\nIndice fuera de rango en lista.'
ACTUAL_TYPED_CATCH_RUNTIME="$($BIN_PATH "$TMP_DIR/typed_catch_runtime.clot")"
if [[ "$ACTUAL_TYPED_CATCH_RUNTIME" != "$EXPECTED_TYPED_CATCH_RUNTIME" ]]; then
    echo "Fallo test typed_catch_runtime" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_TYPED_CATCH_RUNTIME" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_TYPED_CATCH_RUNTIME" >&2
    exit 1
fi

cat > "$TMP_DIR/typed_catch_without_binding.clot" <<'PROG'
try:
    nums = [1, 2];
    println(nums[9]);
catch(IndexError):
    println("typed-no-binding");
endtry
PROG

EXPECTED_TYPED_CATCH_WITHOUT_BINDING=$'typed-no-binding'
ACTUAL_TYPED_CATCH_WITHOUT_BINDING="$($BIN_PATH "$TMP_DIR/typed_catch_without_binding.clot")"
if [[ "$ACTUAL_TYPED_CATCH_WITHOUT_BINDING" != "$EXPECTED_TYPED_CATCH_WITHOUT_BINDING" ]]; then
    echo "Fallo test typed_catch_without_binding" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_TYPED_CATCH_WITHOUT_BINDING" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_TYPED_CATCH_WITHOUT_BINDING" >&2
    exit 1
fi

cat > "$TMP_DIR/typed_catch_hierarchy.clot" <<'PROG'
class Exception:
    public string message;

    constructor(message: string):
        this.message = message;
    endconstructor
endclass

class TypeError extends Exception:
    constructor(message: string):
        super(message);
    endconstructor
endclass

try:
    throw(TypeError("bad-type"));
catch(Exception ex):
    println(ex.message);
endtry
PROG

EXPECTED_TYPED_CATCH_HIERARCHY=$'bad-type'
ACTUAL_TYPED_CATCH_HIERARCHY="$($BIN_PATH "$TMP_DIR/typed_catch_hierarchy.clot")"
if [[ "$ACTUAL_TYPED_CATCH_HIERARCHY" != "$EXPECTED_TYPED_CATCH_HIERARCHY" ]]; then
    echo "Fallo test typed_catch_hierarchy" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_TYPED_CATCH_HIERARCHY" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_TYPED_CATCH_HIERARCHY" >&2
    exit 1
fi

mkdir -p "$TMP_DIR/clot/core/exceptions"
cp "$REPO_ROOT/clot/core/exceptions/exceptions.clot" "$TMP_DIR/clot/core/exceptions/exceptions.clot"

cat > "$TMP_DIR/uncaught_throw_func_object.clot" <<'PROG'
import clot.core.exceptions;

func fail():
    throw(Exception("Something went wrong"));
endfunc

fail();
PROG

set +e
"$BIN_PATH" "$TMP_DIR/uncaught_throw_func_object.clot" >"$TMP_DIR/uncaught_throw_func_object.out" 2>"$TMP_DIR/uncaught_throw_func_object.err"
STATUS_UNCAUGHT_THROW_FUNC_OBJECT=$?
set -e

if [[ "$STATUS_UNCAUGHT_THROW_FUNC_OBJECT" -eq 0 ]]; then
    echo "Fallo test uncaught_throw_func_object: se esperaba fallo de ejecucion." >&2
    exit 1
fi

if ! grep -q "Excepcion no capturada: Exception: Something went wrong" "$TMP_DIR/uncaught_throw_func_object.err"; then
    echo "Fallo test uncaught_throw_func_object: mensaje esperado no encontrado." >&2
    echo "stderr actual:" >&2
    cat "$TMP_DIR/uncaught_throw_func_object.err" >&2
    exit 1
fi

cat > "$TMP_DIR/typed_catch_core_module.clot" <<'PROG'
import clot.core.exceptions;

try:
    throw(TypeError("module-type-error"));
catch(RuntimeError err):
    println(err.message);
endtry
PROG

EXPECTED_TYPED_CATCH_CORE_MODULE=$'module-type-error'
ACTUAL_TYPED_CATCH_CORE_MODULE="$($BIN_PATH "$TMP_DIR/typed_catch_core_module.clot")"
if [[ "$ACTUAL_TYPED_CATCH_CORE_MODULE" != "$EXPECTED_TYPED_CATCH_CORE_MODULE" ]]; then
    echo "Fallo test typed_catch_core_module" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_TYPED_CATCH_CORE_MODULE" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_TYPED_CATCH_CORE_MODULE" >&2
    exit 1
fi

cat > "$TMP_DIR/typed_catch_core_hierarchy.clot" <<'PROG'
import clot.core.exceptions;

try:
    throw(ArgumentError("bad-args"));
catch(TypeError err):
    println(err.message);
endtry

try:
    throw(IndexError("bad-index"));
catch(RangeError err):
    println(err.message);
endtry

try:
    throw(MissingArgumentError("missing-arg"));
catch(ArgumentError err):
    println(err.message);
endtry

try:
    throw(TooManyArgumentsError("too-many-args"));
catch(ArgumentError err):
    println(err.message);
endtry

try:
    throw(FileNotFoundError("file-not-found"));
catch(IOError err):
    println(err.message);
endtry

try:
    throw(ModuleNotFoundError("module-not-found"));
catch(ImportError err):
    println(err.message);
endtry
PROG

EXPECTED_TYPED_CATCH_CORE_HIERARCHY=$'bad-args\nbad-index\nmissing-arg\ntoo-many-args\nfile-not-found\nmodule-not-found'
ACTUAL_TYPED_CATCH_CORE_HIERARCHY="$($BIN_PATH "$TMP_DIR/typed_catch_core_hierarchy.clot")"
if [[ "$ACTUAL_TYPED_CATCH_CORE_HIERARCHY" != "$EXPECTED_TYPED_CATCH_CORE_HIERARCHY" ]]; then
    echo "Fallo test typed_catch_core_hierarchy" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_TYPED_CATCH_CORE_HIERARCHY" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_TYPED_CATCH_CORE_HIERARCHY" >&2
    exit 1
fi

cat > "$TMP_DIR/typed_catch_new_runtime_types.clot" <<'PROG'
try:
    println(missing_symbol);
catch(NameError err):
    println("name");
endtry

try:
    obj = {name: "Ada"};
    println(obj.age);
catch(AttributeError err):
    println("attr");
endtry

try:
    type();
catch(MissingArgumentError err):
    println("missing-arg");
endtry

try:
    input("a", "b");
catch(TooManyArgumentsError err):
    println("too-many");
endtry

try:
    sleep_ms(-1);
catch(RangeError err):
    println("range");
endtry

try:
    read_file("missing-file-for-ioerror-test.txt");
catch(FileNotFoundError err):
    println("file-not-found");
endtry

try:
    import ghost.module;
catch(ModuleNotFoundError err):
    println("module-not-found");
endtry

try:
    println(another_missing_symbol);
catch(RuntimeError err):
    println("runtime-super");
endtry
PROG

EXPECTED_TYPED_CATCH_NEW_RUNTIME_TYPES=$'name\nattr\nmissing-arg\ntoo-many\nrange\nfile-not-found\nmodule-not-found\nruntime-super'
ACTUAL_TYPED_CATCH_NEW_RUNTIME_TYPES="$($BIN_PATH "$TMP_DIR/typed_catch_new_runtime_types.clot")"
if [[ "$ACTUAL_TYPED_CATCH_NEW_RUNTIME_TYPES" != "$EXPECTED_TYPED_CATCH_NEW_RUNTIME_TYPES" ]]; then
    echo "Fallo test typed_catch_new_runtime_types" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_TYPED_CATCH_NEW_RUNTIME_TYPES" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_TYPED_CATCH_NEW_RUNTIME_TYPES" >&2
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

cat > "$TMP_DIR/new_builtins_sequences.clot" <<'PROG'
println(len([10, 20, 30]));
println(len("hola"));
println(range(5));
println(range(1, 6, 2));
println(enumerate(["a", "b"], 5));
println(zip([1, 2, 3], "ab"));
println(all([1, true, "x"]));
println(all([1, 0, 2]));
println(any([0, "", false]));
println(any([0, "", 7]));
PROG

EXPECTED_NEW_BUILTINS_SEQUENCES=$'3\n4\n[0, 1, 2, 3, 4]\n[1, 3, 5]\n[(5, "a"), (6, "b")]\n[(1, \'a\'), (2, \'b\')]\ntrue\nfalse\nfalse\ntrue'
ACTUAL_NEW_BUILTINS_SEQUENCES="$($BIN_PATH "$TMP_DIR/new_builtins_sequences.clot")"
if [[ "$ACTUAL_NEW_BUILTINS_SEQUENCES" != "$EXPECTED_NEW_BUILTINS_SEQUENCES" ]]; then
    echo "Fallo test new_builtins_sequences" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_NEW_BUILTINS_SEQUENCES" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_NEW_BUILTINS_SEQUENCES" >&2
    exit 1
fi

cat > "$TMP_DIR/new_builtins_runtime.clot" <<'PROG'
println(isinstance(10, "int"));
println(isinstance(10, "number"));
println(isinstance("x", ["int", "string"]));
println(chr(65));
println(ord('A'));
println(hex(255));
println(bin(10));
println(hex(-26));
println(bin(-5));
println(hash(tuple(1, 2, 3)) == hash(tuple(1, 2, 3)));
m = map("a", 1);
println(id(m) == id(m));
println(id(map("a", 1)) == id(map("a", 1)));
PROG

EXPECTED_NEW_BUILTINS_RUNTIME=$'true\ntrue\ntrue\nA\n65\n0xff\n0b1010\n-0x1a\n-0b101\ntrue\ntrue\ntrue'
ACTUAL_NEW_BUILTINS_RUNTIME="$($BIN_PATH "$TMP_DIR/new_builtins_runtime.clot")"
if [[ "$ACTUAL_NEW_BUILTINS_RUNTIME" != "$EXPECTED_NEW_BUILTINS_RUNTIME" ]]; then
    echo "Fallo test new_builtins_runtime" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_NEW_BUILTINS_RUNTIME" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_NEW_BUILTINS_RUNTIME" >&2
    exit 1
fi

cat > "$TMP_DIR/new_builtins_range_error.clot" <<'PROG'
range(0, 10, 0);
PROG

set +e
"$BIN_PATH" "$TMP_DIR/new_builtins_range_error.clot" >"$TMP_DIR/new_builtins_range_error.out" 2>"$TMP_DIR/new_builtins_range_error.err"
STATUS_NEW_BUILTINS_RANGE_ERROR=$?
set -e

if [[ "$STATUS_NEW_BUILTINS_RANGE_ERROR" -eq 0 ]]; then
    echo "Fallo test new_builtins_range_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "range() requiere step != 0." "$TMP_DIR/new_builtins_range_error.err"; then
    echo "Fallo test new_builtins_range_error: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/new_builtins_range_error.err" >&2
    exit 1
fi

cat > "$TMP_DIR/new_builtins_ord_error.clot" <<'PROG'
ord("AB");
PROG

set +e
"$BIN_PATH" "$TMP_DIR/new_builtins_ord_error.clot" >"$TMP_DIR/new_builtins_ord_error.out" 2>"$TMP_DIR/new_builtins_ord_error.err"
STATUS_NEW_BUILTINS_ORD_ERROR=$?
set -e

if [[ "$STATUS_NEW_BUILTINS_ORD_ERROR" -eq 0 ]]; then
    echo "Fallo test new_builtins_ord_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "ord() requiere char o string de longitud 1." "$TMP_DIR/new_builtins_ord_error.err"; then
    echo "Fallo test new_builtins_ord_error: mensaje esperado en espanol no encontrado." >&2
    cat "$TMP_DIR/new_builtins_ord_error.err" >&2
    exit 1
fi

set +e
CLOT_LANG=en "$BIN_PATH" "$TMP_DIR/new_builtins_ord_error.clot" >"$TMP_DIR/new_builtins_ord_error_en.out" 2>"$TMP_DIR/new_builtins_ord_error_en.err"
STATUS_NEW_BUILTINS_ORD_ERROR_EN=$?
set -e

if [[ "$STATUS_NEW_BUILTINS_ORD_ERROR_EN" -eq 0 ]]; then
    echo "Fallo test new_builtins_ord_error_en: se esperaba error." >&2
    exit 1
fi

if ! grep -q "ord() requires char or a string of length 1." "$TMP_DIR/new_builtins_ord_error_en.err"; then
    echo "Fallo test new_builtins_ord_error_en: mensaje esperado en ingles no encontrado." >&2
    cat "$TMP_DIR/new_builtins_ord_error_en.err" >&2
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

cat > "$TMP_DIR/control_flow_keywords.clot" <<'PROG'
println("loop");
sum = 0;
for (int i = 0; i < 6; i++):
    if(i == 4):
        break;
    endif
    if(i == 1):
        continue;
    endif
    sum += i;
endfor
println(sum);

acc = 0;
for (n in [2, 3, 4]):
    acc += n;
endfor
println(acc);

switch(acc):
    case 9:
        println("nine");
    case 10:
        println("ten");
        break;
    default:
        println("other");
endswitch

i = 0;
do:
    i += 1;
while(i < 3);
println(i);

if(true):
    pass;
endif
println("pass");

println(3 in [1, 2, 3]);
println("na" in "banana");

const long c = 7;
println(c);

func deferred_demo():
    defer println("defer-last");
    println("defer-now");
endfunc

deferred_demo();

try:
    throw("boom");
catch(err):
    println(err);
finally:
    println("finally");
endtry
PROG

EXPECTED_CONTROL_FLOW_KEYWORDS=$'loop\n5\n9\nnine\nten\n3\npass\ntrue\ntrue\n7\ndefer-now\ndefer-last\nboom\nfinally'
ACTUAL_CONTROL_FLOW_KEYWORDS="$($BIN_PATH "$TMP_DIR/control_flow_keywords.clot")"
if [[ "$ACTUAL_CONTROL_FLOW_KEYWORDS" != "$EXPECTED_CONTROL_FLOW_KEYWORDS" ]]; then
    echo "Fallo test control_flow_keywords" >&2
    echo "Esperado:" >&2
    printf '%s\n' "$EXPECTED_CONTROL_FLOW_KEYWORDS" >&2
    echo "Actual:" >&2
    printf '%s\n' "$ACTUAL_CONTROL_FLOW_KEYWORDS" >&2
    exit 1
fi

cat > "$TMP_DIR/const_reassign_error.clot" <<'PROG'
const int x = 1;
x = 2;
PROG

set +e
"$BIN_PATH" "$TMP_DIR/const_reassign_error.clot" >"$TMP_DIR/const_reassign_error.out" 2>"$TMP_DIR/const_reassign_error.err"
STATUS_CONST_REASSIGN=$?
set -e

if [[ "$STATUS_CONST_REASSIGN" -eq 0 ]]; then
    echo "Fallo test const_reassign_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "No se puede modificar constante: x" "$TMP_DIR/const_reassign_error.err"; then
    echo "Fallo test const_reassign_error: mensaje esperado no encontrado." >&2
    cat "$TMP_DIR/const_reassign_error.err" >&2
    exit 1
fi

set +e
"$BIN_PATH" "$TMP_DIR/const_reassign_error.clot" --lang en >"$TMP_DIR/const_reassign_error_en.out" 2>"$TMP_DIR/const_reassign_error_en.err"
STATUS_CONST_REASSIGN_EN=$?
set -e

if [[ "$STATUS_CONST_REASSIGN_EN" -eq 0 ]]; then
    echo "Fallo test const_reassign_error_en: se esperaba error." >&2
    exit 1
fi

if ! grep -q "Runtime error: Cannot modify constant: x" "$TMP_DIR/const_reassign_error_en.err"; then
    echo "Fallo test const_reassign_error_en: mensaje esperado en ingles no encontrado." >&2
    cat "$TMP_DIR/const_reassign_error_en.err" >&2
    exit 1
fi

cat > "$TMP_DIR/for_invalid_control_header.clot" <<'PROG'
for (int i = 0; i < 3; break):
    pass;
endfor
PROG

set +e
"$BIN_PATH" "$TMP_DIR/for_invalid_control_header.clot" --lang en >"$TMP_DIR/for_invalid_control_header.out" 2>"$TMP_DIR/for_invalid_control_header.err"
STATUS_FOR_INVALID_CONTROL_HEADER=$?
set -e

if [[ "$STATUS_FOR_INVALID_CONTROL_HEADER" -eq 0 ]]; then
    echo "Fallo test for_invalid_control_header: se esperaba error." >&2
    exit 1
fi

if ! grep -q "Parse error at line 1, column 24: for init/update only allows declarations, mutations, or simple expressions." "$TMP_DIR/for_invalid_control_header.err"; then
    echo "Fallo test for_invalid_control_header: mensaje esperado en ingles no encontrado." >&2
    cat "$TMP_DIR/for_invalid_control_header.err" >&2
    exit 1
fi

cat > "$TMP_DIR/in_membership_type_error.clot" <<'PROG'
println(1 in 2);
PROG

set +e
"$BIN_PATH" "$TMP_DIR/in_membership_type_error.clot" --lang en >"$TMP_DIR/in_membership_type_error.out" 2>"$TMP_DIR/in_membership_type_error.err"
STATUS_IN_MEMBERSHIP_TYPE_ERROR=$?
set -e

if [[ "$STATUS_IN_MEMBERSHIP_TYPE_ERROR" -eq 0 ]]; then
    echo "Fallo test in_membership_type_error: se esperaba error." >&2
    exit 1
fi

if ! grep -q "Runtime error: Operator 'in' requires list, tuple, set, map, object, or string on the right-hand side." "$TMP_DIR/in_membership_type_error.err"; then
    echo "Fallo test in_membership_type_error: mensaje esperado en ingles no encontrado." >&2
    cat "$TMP_DIR/in_membership_type_error.err" >&2
    exit 1
fi

echo "Smoke tests OK"
