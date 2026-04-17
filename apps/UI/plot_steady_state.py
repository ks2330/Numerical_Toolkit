"""
Visualise the steady-state temperature field produced by fem_steady_state.

Usage
-----
1. Build and run the C++ executable from the project root:
       cmake --build build
       ./build/apps/UI/fem_steady_state        (Linux/Mac)
       build/apps/UI/fem_steady_state.exe       (Windows)
2. Run this script from the same directory as the generated CSVs:
       python apps/UI/plot_steady_state.py

Output: steady_state_plot.png saved alongside the CSVs.

This file is part of NumericalToolkit, a collection of numerical methods implemented in C++ and Python.

This Visualisation tool was written by AI and is simply a demonstration of how to read and plot the results from the finite element method. It is not intended to be a general-purpose plotting tool, but rather a specific example for this project.


"""



import csv
import sys
import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.tri as tri


# ── helpers ─────────────────────────────────────────────────────────────────

def read_nodes(path: str):
    xs, ys, Ts = [], [], []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
            Ts.append(float(row["temperature"]))
    return np.array(xs), np.array(ys), np.array(Ts)


def read_elements(path: str):
    triangles = []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            triangles.append([int(row["n0"]), int(row["n1"]), int(row["n2"])])
    return np.array(triangles, dtype=int)


# ── main ─────────────────────────────────────────────────────────────────────

def main():
    nodes_csv = "steady_state_nodes.csv"
    elems_csv = "steady_state_elements.csv"

    for path in (nodes_csv, elems_csv):
        if not os.path.exists(path):
            sys.exit(
                f"ERROR: '{path}' not found.\n"
                "Build and run fem_steady_state first to generate the data files."
            )

    x, y, T = read_nodes(nodes_csv)
    triangles = read_elements(elems_csv)

    triang = tri.Triangulation(x, y, triangles)

    # ── figure ───────────────────────────────────────────────────────────────
    fig, ax = plt.subplots(figsize=(11, 4))

    # Smooth Gouraud-shaded temperature field
    tc = ax.tripcolor(triang, T, shading="gouraud", cmap="coolwarm", vmin=0, vmax=100)

    # Overlay mesh skeleton
    ax.triplot(triang, color="black", linewidth=0.6, alpha=0.35)

    # Node scatter coloured by temperature
    sc = ax.scatter(x, y, c=T, cmap="coolwarm", vmin=0, vmax=100,
                    s=40, edgecolors="black", linewidths=0.5, zorder=3)

    # Contour lines at fixed temperature intervals
    levels = np.linspace(0, 100, 7)
    ax.tricontour(triang, T, levels=levels, colors="white",
                  linewidths=0.8, alpha=0.7)

    # Colour bar
    cbar = fig.colorbar(tc, ax=ax, pad=0.02)
    cbar.set_label("Temperature (°C)", fontsize=12)
    cbar.set_ticks(levels)

    # Annotations
    ax.set_title("Steady-State Heat Distribution — FEM (2×6 mesh, Laplace eq.)", fontsize=13)
    ax.set_xlabel("x  (m)", fontsize=11)
    ax.set_ylabel("y  (m)", fontsize=11)
    ax.set_aspect("equal")
    ax.set_xlim(-0.15, 6.15)
    ax.set_ylim(-0.25, 2.25)

    # Boundary labels
    ax.text(-0.1, 1.0, "T = 100 °C", ha="right", va="center",
            fontsize=10, color="darkred", rotation=90)
    ax.text(6.1, 1.0, "T = 0 °C", ha="left", va="center",
            fontsize=10, color="navy", rotation=90)

    plt.tight_layout()

    out = "steady_state_plot.png"
    plt.savefig(out, dpi=150)
    print(f"Plot saved to {out}")
    plt.show()


if __name__ == "__main__":
    main()
