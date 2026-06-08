#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── Configure ─────────────────────────────────────────────────────────────────
echo "[1/3] Configuring CMake..."
cmake -S . -B build

# ── Build ─────────────────────────────────────────────────────────────────────
echo "[2/3] Building all targets..."
cmake --build build

# ── Run tests via CTest ───────────────────────────────────────────────────────
echo ""
echo "[3/3] Running toolkit tests..."
ctest --test-dir build --output-on-failure -C Debug -L toolkit
