#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── Configure ─────────────────────────────────────────────────────────────────
echo "[1/3] Configuring CMake (debug preset)..."
cmake --preset debug

# ── Build ─────────────────────────────────────────────────────────────────────
echo "[2/3] Building..."
cmake --build --preset debug

# ── Test ──────────────────────────────────────────────────────────────────────
echo ""
echo "[3/3] Running toolkit tests..."
ctest --preset debug
