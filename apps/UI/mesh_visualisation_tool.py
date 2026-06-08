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
    ax1.set_xticklabels([f"{v * 10}°" for v in labels_b], rotation=45, ha="right")
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
    ax2.set_xticklabels([f"{v * 10}–{v * 10 + 10}" for v in labels_b], rotation=45, ha="right")
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

    xs, ys, cp_vals = [], [], []
    with open(cp_path, newline="") as f:
        for row in csv.DictReader(f):
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
            cp_vals.append(float(row["Cp"]))
    xs, ys, cp_vals = np.array(xs), np.array(ys), np.array(cp_vals)

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

    BG      = '#0d1117'
    C_AF    = '#58a6ff'
    C_UPPER = '#3fb950'
    C_LOWER = '#f0883e'
    C_GRID  = '#30363d'
    C_TEXT  = '#8b949e'
    CHORD   = 1.0

    # Identify aerofoil surface nodes by exact coordinate match against the DAT file.
    # Coordinates are rounded to 5 d.p. to survive C++ stream-formatting → CSV round-trip.
    dat_path  = "results/dat/aerfoil.dat"
    surf_set: set = set()
    if os.path.exists(dat_path):
        with open(dat_path) as _f:
            for _line in _f:
                _parts = _line.split()
                if len(_parts) == 2:
                    try:
                        surf_set.add((round(float(_parts[0]), 5),
                                      round(float(_parts[1]), 5)))
                    except ValueError:
                        pass

    surf_mask = np.array(
        [(round(xi, 5), round(yi, 5)) in surf_set for xi, yi in zip(xs, ys)]
    )

    xs_s  = xs[surf_mask]
    ys_s  = ys[surf_mask]
    cp_s  = cp_vals[surf_mask]
    xc_s  = xs_s / CHORD
    upper = ys_s >= 0
    u_ord = np.argsort(xc_s[upper])
    l_ord = np.argsort(xc_s[~upper])

    fig, (ax_flow, ax_cp) = plt.subplots(
        1, 2, figsize=(18, 7),
        gridspec_kw={'width_ratios': [1.35, 1]},
    )
    fig.patch.set_facecolor(BG)

    # ── Left: Cp field ────────────────────────────────────────────────────────
    ax_flow.set_facecolor(BG)
    if tris:
        tcf = ax_flow.tricontourf(xs, ys, tris, cp_vals, levels=50, cmap="RdBu_r")
    else:
        tcf = ax_flow.tricontourf(xs, ys, cp_vals, levels=50, cmap="RdBu_r")

    cbar = fig.colorbar(tcf, ax=ax_flow, pad=0.02)
    cbar.set_label("Cₚ", color=C_TEXT)
    cbar.ax.tick_params(labelcolor=C_TEXT, color=C_TEXT)
    cbar.outline.set_edgecolor(C_GRID)

    # Aerofoil silhouette built from the actual surface nodes
    ux = xc_s[upper][u_ord] * CHORD
    uy = ys_s[upper][u_ord]
    lx = xc_s[~upper][l_ord] * CHORD
    ly = ys_s[~upper][l_ord]
    ax_flow.fill(np.concatenate([ux[::-1], lx]),
                 np.concatenate([uy[::-1], ly]),
                 facecolor=BG, edgecolor=C_AF, lw=1.2, zorder=4)

    dot_flow = ax_flow.plot([], [], 'o', color='white', ms=7, zorder=11,
                             markeredgecolor=C_AF, markeredgewidth=1.0)[0]
    ax_flow.set_aspect('equal')
    ax_flow.set_title("Potential Flow — Pressure Coefficient", color='white', fontsize=12)
    ax_flow.set_xlabel("x", color=C_TEXT)
    ax_flow.set_ylabel("y", color=C_TEXT)
    for sp in ax_flow.spines.values():
        sp.set_edgecolor(C_GRID)
    ax_flow.tick_params(colors=C_TEXT)

    # ── Right: Cp distribution ────────────────────────────────────────────────
    ax_cp.set_facecolor(BG)
    ax_cp.plot(xc_s[upper][u_ord],  cp_s[upper][u_ord],
               color=C_UPPER, lw=1.5, label='Upper surface', zorder=3)
    ax_cp.plot(xc_s[~upper][l_ord], cp_s[~upper][l_ord],
               color=C_LOWER, lw=1.5, label='Lower surface', zorder=3)
    ax_cp.scatter(xc_s[upper],  cp_s[upper],  s=22, color=C_UPPER, zorder=5)
    ax_cp.scatter(xc_s[~upper], cp_s[~upper], s=22, color=C_LOWER, zorder=5)
    ax_cp.axhline(0, color=C_GRID, lw=0.8, ls='--', zorder=2)
    ax_cp.invert_yaxis()
    ax_cp.set_xlim(-0.02, 1.02)
    ax_cp.set_xlabel("x/c", color=C_TEXT, fontsize=11)
    ax_cp.set_ylabel("Cₚ", color=C_TEXT, fontsize=11)
    ax_cp.set_title("Cp Distribution — Aerofoil Surface", color='white', fontsize=12)
    for sp in ax_cp.spines.values():
        sp.set_edgecolor(C_GRID)
    ax_cp.tick_params(colors=C_TEXT)
    ax_cp.grid(True, color=C_GRID, lw=0.5, alpha=0.5)
    leg = ax_cp.legend(facecolor='#21262d', edgecolor=C_GRID, fontsize=10)
    for t in leg.get_texts():
        t.set_color('white')

    # ── Hover crosshair ───────────────────────────────────────────────────────
    vline    = ax_cp.axvline(x=0, color='white', lw=0.8, alpha=0.7, zorder=10, visible=False)
    dot_cp_h = ax_cp.plot([], [], 'o', color='white', ms=7, zorder=11,
                           markeredgecolor=C_AF, markeredgewidth=1.0)[0]

    def _hover(event):
        if event.inaxes is ax_flow and event.xdata is not None:
            idx = int(np.argmin(np.hypot(xs_s - event.xdata, ys_s - event.ydata)))
        elif event.inaxes is ax_cp and event.xdata is not None:
            idx = int(np.argmin(np.abs(xc_s - event.xdata)))
        else:
            vline.set_visible(False)
            dot_cp_h.set_data([], [])
            dot_flow.set_data([], [])
            fig.canvas.draw_idle()
            return
        vline.set_visible(True)
        vline.set_xdata([xc_s[idx], xc_s[idx]])
        dot_cp_h.set_data([xc_s[idx]], [cp_s[idx]])
        dot_flow.set_data([xs_s[idx]], [ys_s[idx]])
        fig.canvas.draw_idle()

    fig.canvas.mpl_connect('motion_notify_event', _hover)

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
    os.makedirs(os.path.dirname(out), exist_ok=True)
    plt.savefig(out, dpi=150)
    print(f"Plot saved to {out}")
    plt.show()

    plot_mesh_quality_comparison()
    plot_pressure_field()


if __name__ == "__main__":
    main()
