"""
Visualise mesh generation output from fem_steady_state.

Plots:
  - Rectangle boundary + interior nodes  (boundary_nodes_rectangular.csv)
  - Bowyer-Watson triangulation edges    (triangulation.csv)
  - Inscribed circle                     (inscribed_circle.csv)
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


def draw_triangulation(ax, path):
    if not os.path.exists(path):
        print(f"WARNING: '{path}' not found — skipping.")
        return
    first = True
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            xs = [float(row["ax"]), float(row["bx"]), float(row["cx"]), float(row["ax"])]
            ys = [float(row["ay"]), float(row["by"]), float(row["cy"]), float(row["ay"])]
            ax.plot(xs, ys, color="purple", linewidth=1.5, alpha=0.7,
                    label="Triangulation" if first else None)
            first = False


def draw_inscribed_circle(ax, path):
    if not os.path.exists(path):
        print(f"WARNING: '{path}' not found — skipping.")
        return
    with open(path, newline="") as f:
        row = next(csv.DictReader(f))
        cx = float(row["cx"])
        cy = float(row["cy"])
        r  = float(row["radius"])
    circle = plt.Circle((cx, cy), r, color="crimson", fill=False,
                        linewidth=2.0, linestyle="--", label="Inscribed circle")
    ax.add_patch(circle)
    ax.plot(cx, cy, "r+", markersize=8, zorder=4)


# ── main ─────────────────────────────────────────────────────────────────────
def main():
    datasets = [
        ("results/csv/boundary_nodes_rectangular.csv", "Mesh nodes", "steelblue", 40, "o"),
    ]

    fig, ax = plt.subplots(figsize=(11, 6))

    draw_triangulation(ax, "results/csv/triangulation.csv")
    draw_inscribed_circle(ax, "results/csv/inscribed_circle.csv")

    for path, label, colour, size, marker in datasets:
        if not os.path.exists(path):
            print(f"WARNING: '{path}' not found — skipping.")
            continue
        x, y = read_xy(path)
        ax.scatter(x, y, c=colour, s=size, marker=marker,
                   edgecolors="black", linewidths=0.4, zorder=3, label=label)

    ax.set_title("Mesh visualisation — Delaunay triangulation with inscribed circle", fontsize=13)
    ax.set_xlabel("x", fontsize=11)
    ax.set_ylabel("y", fontsize=11)
    ax.set_aspect("equal")
    ax.legend(fontsize=10)
    ax.grid(True, linestyle="--", alpha=0.4)

    plt.tight_layout()
    out = "results/png/mesh_visualisation.png"
    plt.savefig(out, dpi=150)
    print(f"Plot saved to {out}")
    plt.show()


if __name__ == "__main__":
    main()
