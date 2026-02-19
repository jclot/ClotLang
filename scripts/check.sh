#!/usr/bin/env bash
set -euo pipefail

FORMAT_CHECK=0

for arg in "$@"; do
  case "$arg" in
    --format-check)
      FORMAT_CHECK=1
      ;;
    *)
      echo "Unknown option: $arg" >&2
      echo "Usage: scripts/check.sh [--format-check]" >&2
      exit 1
      ;;
  esac
done

if [[ "$FORMAT_CHECK" -eq 1 ]]; then
  if ! command -v clang-format >/dev/null 2>&1; then
    echo "clang-format not found. Install clang-format to run --format-check." >&2
    exit 1
  fi

  mapfile -t FILES < <(find include src -type f \( -name '*.hpp' -o -name '*.cpp' \) | sort)
  if [[ "${#FILES[@]}" -eq 0 ]]; then
    echo "No C++ files found for format check."
  else
    clang-format --dry-run --Werror "${FILES[@]}"
  fi
fi

cmake --preset wsl-release
cmake --build --preset build-release
tests/smoke.sh ./build/wsl-release/clot
tests/llvm_smoke.sh ./build/wsl-release/clot

echo "All checks passed."
