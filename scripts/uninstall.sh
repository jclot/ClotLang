#!/usr/bin/env bash
set -euo pipefail

PREFIX="${CLOT_PREFIX:-$HOME/.local}"
BIN_DIR="${CLOT_BIN_DIR:-$PREFIX/bin}"

usage() {
  cat <<'EOF'
Clot uninstaller (Linux/macOS)

Usage:
  uninstall.sh [--prefix <dir>] [--bin-dir <dir>]

Env:
  CLOT_PREFIX   Install prefix (default: ~/.local)
  CLOT_BIN_DIR  Bin directory (default: <prefix>/bin)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --prefix)
      PREFIX="$2"
      BIN_DIR="$PREFIX/bin"
      shift 2
      ;;
    --bin-dir)
      BIN_DIR="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 1
      ;;
  esac
done

target="$BIN_DIR/clot"
if [[ -f "$target" ]]; then
  rm -f "$target"
  echo "Removed: $target"
else
  echo "Not found: $target"
fi

if [[ -d "$BIN_DIR" ]]; then
  if [[ -z "$(ls -A "$BIN_DIR" 2>/dev/null || true)" ]]; then
    rmdir "$BIN_DIR" 2>/dev/null || true
  fi
fi

echo "Uninstall complete."
if echo ":$PATH:" | grep -q ":$BIN_DIR:"; then
  echo "Note: remove $BIN_DIR from PATH if you no longer need it."
fi
