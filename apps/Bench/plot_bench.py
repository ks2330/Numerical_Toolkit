"""
Plot FEM potential-flow solver scaling from results/csv/bench.csv.

Produces a log-log plot of median solve time vs. number of nodes, one line per
solver, with N^1 / N^2 / N^3 reference slopes so the asymptotic cost of each
approach is obvious (dense Gaussian elimination ~ N^3; sparse solvers scale far
better). Run from the repository root after fem_bench has written the CSV:

    ./build/apps/Bench/fem_bench      # writes results/csv/bench.csv
    python apps/Bench/plot_bench.py   # writes results/png/bench_scaling.png
"""

import csv
import os
import sys
from collections import defaultdict

import numpy as np
import matplotlib.pyplot as plt

CSV_PATH = "results/csv/bench.csv"
OUT_PATH = "results/png/bench_scaling.png"


def read_bench(path):
    """Return {solver_name: (N_array_sorted, median_seconds_array)}."""
    if not os.path.exists(path):
        sys.exit(f"ERROR: '{path}' not found. Build and run fem_bench first.")
    by_solver = defaultdict(list)
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            by_solver[row["solver"]].append(
                (int(row["num_nodes"]), float(row["median_s"]))
            )
    out = {}
    for name, pairs in by_solver.items():
        pairs.sort()
        N = np.array([p[0] for p in pairs], dtype=float)
        t = np.array([p[1] for p in pairs], dtype=float)
        out[name] = (N, t)
    return out


def add_reference_slope(ax, N, exponent, anchor_t, label):
    """Draw a dashed t ~ N^exponent guide line anchored at (N[0], anchor_t)."""
    ref = anchor_t * (N / N[0]) ** exponent
    ax.plot(N, ref, "--", color="grey", linewidth=1.0, alpha=0.7)
    ax.annotate(label, (N[-1], ref[-1]), color="grey", fontsize=9,
                ha="left", va="center")


def main():
    data = read_bench(CSV_PATH)

    fig, ax = plt.subplots(figsize=(9, 6))
    for name, (N, t) in sorted(data.items()):
        ax.plot(N, t, "o-", linewidth=1.8, markersize=5, label=name)

    # Reference slopes anchored to the smallest problem's fastest solver.
    all_N = np.unique(np.concatenate([N for N, _ in data.values()]))
    if all_N.size >= 2:
        anchor = min(t[0] for _, t in data.values())
        for exp, lbl in ((1, "O(N)"), (2, "O(N^2)"), (3, "O(N^3)")):
            add_reference_slope(ax, all_N, exp, anchor, lbl)

    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Number of nodes (N)")
    ax.set_ylabel("Median solve time (s)")
    ax.set_title("FEM Potential-Flow Solver Scaling: Dense vs. Sparse")
    ax.grid(True, which="both", linestyle="--", alpha=0.3)
    ax.legend(fontsize=9)

    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    fig.tight_layout()
    fig.savefig(OUT_PATH, dpi=150)
    print(f"Scaling plot saved to {OUT_PATH}")
    plt.show()


if __name__ == "__main__":
    main()
