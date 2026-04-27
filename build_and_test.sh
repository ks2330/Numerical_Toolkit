#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── Configure ─────────────────────────────────────────────────────────────────
if [ ! -d "build" ]; then
    echo "[1/2] Configuring CMake..."
    cmake -S . -B build
else
    echo "[1/2] Build directory exists, skipping configure."
fi

# ── Build ─────────────────────────────────────────────────────────────────────
echo "[2/2] Building all targets..."
cmake --build build

# ── Run tests via CTest ───────────────────────────────────────────────────────
echo ""
echo "Running all tests..."
ctest --test-dir build --output-on-failure
