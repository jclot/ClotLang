#!/usr/bin/env bash
set -euo pipefail

BIN_PATH="${1:-./build/wsl-release/clot}"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CASES_DIR="$ROOT_DIR/benchmarks/cases"

if [[ ! -x "$BIN_PATH" ]]; then
    echo "Binario no encontrado o no ejecutable: $BIN_PATH" >&2
    exit 1
fi

run_case_interpret() {
    local file="$1"
    local label="$2"
    /usr/bin/time -f "[interpret] ${label}: %e s" "$BIN_PATH" "$file" >/dev/null
}

run_case_compile() {
    local file="$1"
    local label="$2"
    local tmp_dir
    tmp_dir="$(mktemp -d)"

    local exe_path="$tmp_dir/case.exe"
    if "$BIN_PATH" "$file" --mode compile --emit exe -o "$exe_path" >/dev/null 2>&1; then
        /usr/bin/time -f "[compile]   ${label}: %e s" "$exe_path" >/dev/null
    else
        echo "[compile]   ${label}: fallback/no disponible" >&2
    fi
    rm -rf "$tmp_dir"
}

for file in "$CASES_DIR"/*.clot; do
    label="$(basename "$file")"
    run_case_interpret "$file" "$label"
    run_case_compile "$file" "$label"
done
