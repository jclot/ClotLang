#!/usr/bin/env bash
set -euo pipefail

REPO="jclot/ClotLang"
VERSION="${CLOT_VERSION:-}"
URL="${CLOT_URL:-}"
PREFIX="${CLOT_PREFIX:-$HOME/.local}"
BIN_DIR="${CLOT_BIN_DIR:-$PREFIX/bin}"

usage() {
  cat <<'EOF'
Clot installer (Linux/macOS)

Usage:
  install.sh [--prefix <dir>] [--bin-dir <dir>] [--version <vX.Y.Z>] [--url <asset-url>]

Env:
  CLOT_PREFIX   Install prefix (default: ~/.local)
  CLOT_BIN_DIR  Bin directory (default: <prefix>/bin)
  CLOT_VERSION  Release tag (default: latest)
  CLOT_URL      Full URL to a release asset (overrides version/latest)
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
    --version)
      VERSION="$2"
      shift 2
      ;;
    --url)
      URL="$2"
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

case "$(uname -s)" in
  Linux) OS="linux" ;;
  Darwin) OS="macos" ;;
  *)
    echo "Unsupported OS. Use scripts/install.ps1 on Windows." >&2
    exit 1
    ;;
esac

case "$(uname -m)" in
  x86_64|amd64) ARCH="x86_64" ;;
  arm64|aarch64) ARCH="arm64" ;;
  *)
    echo "Unsupported architecture: $(uname -m)" >&2
    exit 1
    ;;
esac

ASSET="clot-${OS}-${ARCH}.tar.gz"

if [[ -z "$URL" ]]; then
  if [[ -n "$VERSION" ]]; then
    URL="https://github.com/${REPO}/releases/download/${VERSION}/${ASSET}"
  else
    URL="https://github.com/${REPO}/releases/latest/download/${ASSET}"
  fi
fi

if command -v curl >/dev/null 2>&1; then
  download() { curl -fL --retry 3 --retry-delay 2 -o "$1" "$2"; }
elif command -v wget >/dev/null 2>&1; then
  download() { wget -O "$1" "$2"; }
else
  echo "curl or wget is required." >&2
  exit 1
fi

tmp_dir="$(mktemp -d)"
cleanup() { rm -rf "$tmp_dir"; }
trap cleanup EXIT

archive="$tmp_dir/$ASSET"
download "$archive" "$URL"

extract_dir="$tmp_dir/extract"
mkdir -p "$extract_dir"
tar -xzf "$archive" -C "$extract_dir"

bin_path="$(find "$extract_dir" -type f -name clot -perm -u+x 2>/dev/null | head -n 1 || true)"
if [[ -z "$bin_path" ]]; then
  bin_path="$(find "$extract_dir" -type f -name clot 2>/dev/null | head -n 1 || true)"
fi
if [[ -z "$bin_path" ]]; then
  echo "clot binary not found in the archive." >&2
  exit 1
fi

mkdir -p "$BIN_DIR"
cp "$bin_path" "$BIN_DIR/clot"
chmod +x "$BIN_DIR/clot"

echo "Installed: $BIN_DIR/clot"
if ! echo ":$PATH:" | grep -q ":$BIN_DIR:"; then
  echo "Warning: $BIN_DIR is not on PATH."
  echo "Add it with: export PATH=\"$BIN_DIR:\$PATH\""
fi
