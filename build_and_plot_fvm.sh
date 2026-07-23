#!/usr/bin/env bash
# Build fvm_solver, run it from the project root, then plot the FVM Euler solution.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

rm -f results/csv/fvm_pressure_field.csv
rm -f results/csv/fvm_forces.csv
rm -f results/png/fvm_solution.png

mkdir -p results/csv results/png

if [ ! -d "build" ]; then
    echo "[1/4] Configuring CMake..."
    cmake -S . -B build
else
    echo "[1/4] Build directory already exists, skipping configure."
fi

echo "[2/4] Building fvm_solver..."
cmake --build build --target fvm_solver

# EXE=""
for candidate in \
    "build/apps/FVM_solver/fvm_solver.exe" \
    "build/apps/FVM_solver/Debug/fvm_solver.exe" \
    "build/apps/FVM_solver/Release/fvm_solver.exe" \
    "build/apps/FVM_solver/fvm_solver"
do
    if [ -f "$SCRIPT_DIR/$candidate" ]; then
        EXE="$SCRIPT_DIR/$candidate"
        break
    fi
done

if [ -z "$EXE" ]; then
    echo "ERROR: Could not find fvm_solver executable after build." >&2
    exit 1
fi

echo "Found executable: $EXE"

echo "[3/4] Running solver..."
"$EXE"

echo "[4/4] Generating plots..."
python apps/FVM_solver/plot_fvm.py

echo "Done."
echo "  CSVs -> results/csv/"
echo "  PNGs -> results/png/"
