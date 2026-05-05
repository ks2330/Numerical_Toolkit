#!/usr/bin/env bash
# Build fem_steady_state, run it from the project root, then plot.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── Clean previous results ────────────────────────────────────────────────────
rm -f results/csv/boundary_nodes_rectangular.csv
rm -f results/csv/steady_state_nodes.csv
rm -f results/csv/steady_state_elements.csv
rm -f results/csv/triangulation.csv
rm -f results/png/mesh_visualisation.png
rm -f results/png/steady_state_plot.png

# ── 1. Configure ─────────────────────────────────────────────────────────────
if [ ! -d "build" ]; then
    echo "[1/4] Configuring CMake..."
    cmake -S . -B build
else
    echo "[1/4] Build directory already exists, skipping configure."
fi

# ── 2. Build ──────────────────────────────────────────────────────────────────
echo "[2/4] Building fem_steady_state..."
cmake --build build --target fem_steady_state

# Locate the executable
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

# ── 3. Run from project root (outputs land in results/) ───────────────────────
echo "[3/4] Running executable..."
"$EXE"

# ── 4. Plot ───────────────────────────────────────────────────────────────────
echo "[4/4] Generating plots..."
python apps/UI/mesh_visualisation_tool.py
python apps/UI/plot_steady_state.py

echo "Done."
echo "  CSVs -> results/csv/"
echo "  PNGs -> results/png/"
