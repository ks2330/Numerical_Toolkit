"""
Visualise mesh generation output from fem_steady_state.

Plots three datasets in distinct colours:
  - Rectangle boundary + interior nodes  (boundary_nodes_rectangular.csv)
  - Large triangle nodes                 (elements_rectangular.csv)
  - Circumcircle points                  (boundary_nodes_circular.csv)
"""

import csv
import os
import numpy as np
import matplotlib.pyplot as plt


def read_xy(path: str):
    xs, ys = [], []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
    return np.array(xs), np.array(ys)


def main():
    datasets = [
        ("boundary_nodes_rectangular.csv", "Rectangle nodes",  "steelblue",  40, "o"),
        ("elements_rectangular.csv",        "Triangle nodes",   "darkorange", 60, "^"),
        ("boundary_nodes_circular.csv",     "Circumcircle",     "mediumseagreen", 20, "."),
    ]

    fig, ax = plt.subplots(figsize=(11, 6))

    for path, label, colour, size, marker in datasets:
        if not os.path.exists(path):
            print(f"WARNING: '{path}' not found — skipping.")
            continue
        x, y = read_xy(path)
        ax.scatter(x, y, c=colour, s=size, marker=marker,
                   edgecolors="black", linewidths=0.4, zorder=3, label=label)

    ax.set_title("Mesh visualisation — nodes, triangle & circumcircle", fontsize=13)
    ax.set_xlabel("x", fontsize=11)
    ax.set_ylabel("y", fontsize=11)
    ax.set_aspect("equal")
    ax.legend(fontsize=10)
    ax.grid(True, linestyle="--", alpha=0.4)

    plt.tight_layout()
    out = "mesh_visualisation.png"
    plt.savefig(out, dpi=150)
    print(f"Plot saved to {out}")
    plt.show()


if __name__ == "__main__":
    main()
