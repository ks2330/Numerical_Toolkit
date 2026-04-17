"""
Visualise the mesh generation produced by fem_steady_state.

Usage
-----
1. Build and run the C++ executable from the project root:
       cmake --build build
       ./build/apps/UI/fem_steady_state        (Linux/Mac)
       build/apps/UI/fem_steady_state.exe       (Windows)
2. Run this script from the same directory as the generated CSVs:
       python apps/UI/plot_steady_state.py

Output: steady_state_plot.png saved alongside the CSVs.
"""

import csv
import sys
import os
import numpy as np
import matplotlib.pyplot as plt

# ── helpers ─────────────────────────────────────────────────────────────────


def read_nodes(path: str):
    xs, ys, Ts = [], [], []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            Ts.append(float(row["id"]))
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
    return np.array(xs), np.array(ys), np.array(Ts)


# ── main ─────────────────────────────────────────────────────────────────────
def main():
    path = "Boundary_nodes_rectangular.csv"

    if not os.path.exists(path):
        sys.exit(
            f"ERROR: '{path}' not found.\n"
            "Build and run fem_steady_state first to generate the data files."
        )

    x, y, T = read_nodes(path)

    # ── figure ───────────────────────────────────────────────────────────────
    fig, ax = plt.subplots(figsize=(11, 4))

    # Overlay mesh skeleton
    # ax.triplot(color="black", linewidth=0.6, alpha=0.35)
    ax.scatter(
        x,
        y,
        c=T,
        cmap="coolwarm",
        vmin=0,
        vmax=100,
        s=40,
        edgecolors="black",
        linewidths=0.5,
        zorder=3,
    )

    # Add Lines between the points to show the mesh structure
    #    for i in range(len(x)):
    #        for j in range(i + 1, len(x)):
    #            if np.isclose(x[i], x[j], atol=0.01) or np.isclose(y[i], y[j], atol=0.01):
    #                ax.plot(
    #                    [x[i], x[j]],
    #                    [y[i], y[j]],
    #                    color="black",
    #                    linewidth=1.5,
    #                    alpha=0.35,
    #                    zorder=2,
    #                )

    # Annotations
    ax.set_title(
        "Steady-State Heat Distribution — FEM (2×6 mesh, Laplace eq.)", fontsize=13
    )
    ax.set_xlabel("x  (m)", fontsize=11)
    ax.set_ylabel("y  (m)", fontsize=11)
    ax.set_aspect("equal")
    ax.set_xlim(-0.15, 6.15)
    ax.set_ylim(-0.25, 2.25)

    # Boundary labels
    ax.text(
        -0.1,
        1.0,
        "-",
        ha="right",
        va="center",
        fontsize=10,
        color="darkred",
        rotation=90,
    )
    ax.text(
        6.1,
        1.0,
        "-",
        ha="left",
        va="center",
        fontsize=10,
        color="navy",
        rotation=90,
    )

    plt.tight_layout()

    out = "steady_state_plot.png"
    plt.savefig(out, dpi=150)
    print(f"Plot saved to {out}")
    plt.show()


if __name__ == "__main__":
    main()
