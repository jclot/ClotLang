#!/usr/bin/env bash
set -euo pipefail

echo "[clot] Detectando sistema..."
if ! command -v apt-get >/dev/null 2>&1; then
    echo "[clot] Este script esta preparado para Ubuntu/Debian (apt-get)." >&2
    exit 1
fi

echo "[clot] Instalando dependencias LLVM/Clang para WSL..."
sudo apt-get update
sudo apt-get install -y \
    clang \
    lld \
    llvm \
    llvm-dev \
    libclang-dev \
    cmake \
    ninja-build

echo "[clot] Versiones detectadas:"
clang++ --version | head -n 1 || true
llvm-config --version || true

echo "[clot] Listo. Reconfigura y compila el proyecto:"
echo "  cmake -S . -B build -G Ninja -DCLOT_ENABLE_LLVM=ON"
echo "  cmake --build build"
