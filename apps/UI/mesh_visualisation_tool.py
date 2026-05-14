"""
Visualise mesh generation output from fem_steady_state.

Plots:
  - Rectangle boundary + interior nodes  (boundary_nodes_rectangular.csv)
  - Bowyer-Watson triangulation edges    (triangulation.csv)
  - Inscribed circle                     (inscribed_circle.csv)
  - Mesh quality comparison charts       (results/metrics/*.csv)
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


def read_metric(path: str):
    """Read a metrics CSV; returns (bin_labels_as_ints, counts)."""
    labels, counts = [], []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            key = list(row.keys())[0]
            labels.append(int(row[key]))
            counts.append(int(row["Count"]))
    return labels, counts


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


def plot_mesh_quality_comparison():
    """Comparative bar charts: angle and aspect-ratio distributions before/after improvement."""
    metrics_dir = "results/metrics"
    paths = {
        "angle_before":  os.path.join(metrics_dir, "angle_distribution.csv"),
        "angle_after":   os.path.join(metrics_dir, "angle_distribution_improved.csv"),
        "ratio_before":  os.path.join(metrics_dir, "aspect_ratio_distribution.csv"),
        "ratio_after":   os.path.join(metrics_dir, "aspect_ratio_distribution_improved.csv"),
    }

    missing = [p for p in paths.values() if not os.path.exists(p)]
    if missing:
        print(f"WARNING: metric files not found, skipping quality plot: {missing}")
        return

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    w = 0.4

    # ── Angle distribution ────────────────────────────────────────────────────
    labels_b, counts_b = read_metric(paths["angle_before"])
    labels_a, counts_a = read_metric(paths["angle_after"])
    x = np.arange(len(labels_b))

    ax1.bar(x - w / 2, counts_b, width=w, label="Before improvement",
            color="steelblue", alpha=0.85)
    ax1.bar(x + w / 2, counts_a, width=w, label="After improvement",
            color="darkorange", alpha=0.85)
    ax1.set_xticks(x)
    ax1.set_xticklabels([f"{v}°" for v in labels_b], rotation=45, ha="right")
    ax1.set_title("Minimum Angle Distribution", fontsize=13)
    ax1.set_xlabel("Angle bin (min per element)", fontsize=11)
    ax1.set_ylabel("Element count", fontsize=11)
    ax1.legend(fontsize=10)
    ax1.grid(True, axis="y", linestyle="--", alpha=0.4)

    # ── Aspect-ratio distribution ─────────────────────────────────────────────
    labels_b, counts_b = read_metric(paths["ratio_before"])
    labels_a, counts_a = read_metric(paths["ratio_after"])
    x = np.arange(len(labels_b))

    ax2.bar(x - w / 2, counts_b, width=w, label="Before improvement",
            color="steelblue", alpha=0.85)
    ax2.bar(x + w / 2, counts_a, width=w, label="After improvement",
            color="darkorange", alpha=0.85)
    ax2.set_xticks(x)
    ax2.set_xticklabels([f"{v}–{v + 10}" for v in labels_b], rotation=45, ha="right")
    ax2.set_title("Aspect Ratio Distribution", fontsize=13)
    ax2.set_xlabel("Aspect ratio bin", fontsize=11)
    ax2.set_ylabel("Element count", fontsize=11)
    ax2.legend(fontsize=10)
    ax2.grid(True, axis="y", linestyle="--", alpha=0.4)

    fig.suptitle("Mesh Quality: Before vs. After Improvement", fontsize=14, fontweight="bold")
    plt.tight_layout()
    out = "results/png/mesh_quality_comparison.png"
    plt.savefig(out, dpi=150)
    print(f"Quality comparison plot saved to {out}")
    plt.show()


def plot_pressure_field():
    cp_path  = "results/csv/pressure_field.csv"
    tri_path = "results/csv/triangulation.csv"
    if not os.path.exists(cp_path):
        print(f"WARNING: '{cp_path}' not found — skipping pressure field plot.")
        return

    xs, ys, cp = [], [], []
    with open(cp_path, newline="") as f:
        for row in csv.DictReader(f):
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
            cp.append(float(row["Cp"]))
    xs, ys, cp = np.array(xs), np.array(ys), np.array(cp)

    tris = []
    if os.path.exists(tri_path):
        id_map = {(x, y): i for i, (x, y) in enumerate(zip(xs, ys))}
        with open(tri_path, newline="") as f:
            for row in csv.DictReader(f):
                try:
                    a = id_map[(float(row["ax"]), float(row["ay"]))]
                    b = id_map[(float(row["bx"]), float(row["by"]))]
                    c = id_map[(float(row["cx"]), float(row["cy"]))]
                    tris.append([a, b, c])
                except KeyError:
                    pass

    fig, ax = plt.subplots(figsize=(13, 6))
    if tris:
        tcf = ax.tricontourf(xs, ys, tris, cp, levels=50, cmap="RdBu_r")
    else:
        tcf = ax.tricontourf(xs, ys, cp, levels=50, cmap="RdBu_r")
    fig.colorbar(tcf, ax=ax, label="Pressure coefficient Cp")

    ax.set_title("Potential Flow — Pressure Coefficient", fontsize=13)
    ax.set_xlabel("x"); ax.set_ylabel("y")
    ax.set_aspect("equal")
    plt.tight_layout()
    out = "results/png/pressurefield.png"
    plt.savefig(out, dpi=150)
    print(f"Pressure field plot saved to {out}")
    plt.show()


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

    plot_mesh_quality_comparison()
    plot_pressure_field()


if __name__ == "__main__":
    main()
