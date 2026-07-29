#!/usr/bin/env bash
# Build fem_steady_state, run it from apps/UI (so CSVs land there), then plot.
set -euo pipefail

# Ensure .csv files are removed before being generated again.

rm -f boundary_nodes_rectangular.csv
rm -f steady_state_nodes.csv
rm -f steady_state_elements.csv

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── 1. Configure ─────────────────────────────────────────────────────────────
GENERATOR="Ninja"
if [ -f "build/CMakeCache.txt" ] && ! grep -q "CMAKE_GENERATOR:INTERNAL=$GENERATOR" "build/CMakeCache.txt"; then
    echo "[1/3] Existing build/ used a different generator — reconfiguring with $GENERATOR."
    rm -rf build
fi
if [ ! -d "build" ]; then
    echo "[1/3] Configuring CMake ($GENERATOR)..."
    cmake -S . -B build -G "$GENERATOR"
else
    echo "[1/3] Build directory already exists, skipping configure."
fi

# ── 2. Build ──────────────────────────────────────────────────────────────────
echo "[2/3] Building fem_steady_state..."
cmake --build build --target fem_steady_state

# Locate the executable (handles both flat and Debug/Release subdirs)
EXE=""
for candidate in \
    "build/apps/UI/fem_steady_state.exe" \
    "build/apps/UI/Debug/fem_steady_state.exe" \
    "build/apps/UI/Release/fem_steady_state.exe" \
    "build/apps/UI/fem_steady_state"
do
    if [ -f "$SCRIPT_DIR/$candidate" ]; then
        EXE="$SCRIPT_DIR/$candidate"
        break
    fi
done

if [ -z "$EXE" ]; then
    echo "ERROR: Could not find fem_steady_state executable after build." >&2
    exit 1
fi

echo "Found executable: $EXE"

# ── 3. Run from project root so the binary's relative paths resolve correctly ──
echo "[3/3] Running executable and plotting..."
cd "$SCRIPT_DIR"
"$EXE"

echo "Done"
