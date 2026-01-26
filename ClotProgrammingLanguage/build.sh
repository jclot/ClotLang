#!/usr/bin/env bash
set -euo pipefail

# Build script for Clot interpreter
cd "$(dirname "$0")"

g++ -std=c++17 -O2 -pipe -flto *.cpp -o clot

echo "Built: $(pwd)/clot"
echo "Run: ./clot [file.clot]"