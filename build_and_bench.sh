#!/usr/bin/env bash
# Build and run the fem_bench benchmark.
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
rm -f results/png/mesh_quality_comparison.png
rm -f results/metrics/angle_distribution.csv
rm -f results/metrics/aspect_ratio_distribution.csv
rm -f results/metrics/angle_distribution_improved.csv
rm -f results/metrics/aspect_ratio_distribution_improved.csv

mkdir -p results/metrics
# ── 1. Configure ─────────────────────────────────────────────────────────────
if [ ! -d "build" ]; then
    echo "[1/3] Configuring CMake..."
    cmake -S . -B build
else
    echo "[1/3] Build directory already exists, skipping configure."
fi

# ── 2. Build ──────────────────────────────────────────────────────────────────
echo "[2/3] Building fem_bench..."
cmake --build build --target fem_bench

# Locate the executable
EXE=""
for candidate in \
    "build/apps/Bench/fem_bench.exe" \
    "build/apps/Bench/Debug/fem_bench.exe" \
    "build/apps/Bench/Release/fem_bench.exe" \
    "build/apps/Bench/fem_bench"
do
    if [ -f "$SCRIPT_DIR/$candidate" ]; then
        EXE="$SCRIPT_DIR/$candidate"
        break
    fi
done

if [ -z "$EXE" ]; then
    echo "ERROR: Could not find fem_bench executable after build." >&2
    exit 1
fi

echo "Found executable: $EXE"

# ── 3. Run from project root (outputs land in results/) ───────────────────────
echo "[3/3] Running benchmark..."
"$EXE"

echo "Done."
