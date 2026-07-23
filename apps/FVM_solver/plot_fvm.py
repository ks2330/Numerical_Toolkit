import csv
import os
import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.tri as tri


def read_field(path):
    xs, ys, ps, ms = [], [], [], []
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
            ps.append(float(row["pressure"]))
            ms.append(float(row["mach"]))
    return np.array(xs), np.array(ys), np.array(ps), np.array(ms)


def read_forces(path):
    with open(path, newline="") as f:
        return next(csv.DictReader(f))


def read_mesh(nodes_path, elems_path):
    xs, ys = [], []
    with open(nodes_path, newline="") as f:
        for row in csv.DictReader(f):
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
    tris = []
    with open(elems_path, newline="") as f:
        for row in csv.DictReader(f):
            tris.append([int(row["n0"]), int(row["n1"]), int(row["n2"])])
    return np.array(xs), np.array(ys), np.array(tris, dtype=int)


def read_aerofoil(path):
    xs, ys = [], []
    if not os.path.exists(path):
        return None, None
    with open(path) as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 2:
                try:
                    xs.append(float(parts[0]))
                    ys.append(float(parts[1]))
                except ValueError:
                    pass
    return np.array(xs), np.array(ys)


def plot_mesh():
    nodes_csv = "results/csv/fvm_nodes.csv"
    elems_csv = "results/csv/fvm_elements.csv"
    if not (os.path.exists(nodes_csv) and os.path.exists(elems_csv)):
        return

    x, y, tris = read_mesh(nodes_csv, elems_csv)
    triang = tri.Triangulation(x, y, tris)

    fig, ax = plt.subplots(figsize=(11, 7))
    ax.triplot(triang, color="black", linewidth=0.3)

    ax_x, ax_y = read_aerofoil("results/dat/aerofoil.dat")
    if ax_x is not None:
        ax.plot(np.append(ax_x, ax_x[0]), np.append(ax_y, ax_y[0]),
                color="red", linewidth=1.8, label="aerofoil surface")
        ax.legend(loc="upper right")

    ax.set_title(f"FVM mesh check ({len(tris)} cells, {len(x)} nodes) "
                 "- cells inside the red outline = bad")
    ax.set_aspect("equal")
    plt.tight_layout()
    out = "results/png/fvm_mesh.png"
    plt.savefig(out, dpi=150)
    print(f"Mesh check saved to {out}")


def plot_solution():
    field_csv = "results/csv/fvm_pressure_field.csv"
    forces_csv = "results/csv/fvm_forces.csv"
    nodes_csv = "results/csv/fvm_nodes.csv"
    elems_csv = "results/csv/fvm_elements.csv"
    for path in (field_csv, forces_csv, nodes_csv, elems_csv):
        if not os.path.exists(path):
            sys.exit(f"ERROR: '{path}' not found. Build and run fvm_solver first.")

    # Plot the field on the REAL mesh connectivity (one cell-centred value per element),
    # NOT a Delaunay re-triangulation of the centroids. The elements never span the aerofoil
    # hole, so the interior is simply uncovered (clean smooth boundary, no bridging triangles,
    # no mask needed). A first-order FV solution is genuinely piecewise-constant per cell, so
    # flat per-face shading is the honest representation.
    nx, ny, tris = read_mesh(nodes_csv, elems_csv)
    _, _, p, m = read_field(field_csv)            # row i corresponds to element i
    F = read_forces(forces_csv)

    if not (len(p) == len(m) == len(tris)):
        sys.exit(f"ERROR: field rows ({len(p)}) != elements ({len(tris)}); cannot map cell values.")

    triang = tri.Triangulation(nx, ny, tris)
    ax_x, ax_y = read_aerofoil("results/dat/aerofoil.dat")

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    tc = ax1.tripcolor(triang, facecolors=p, cmap="coolwarm")
    fig.colorbar(tc, ax=ax1, pad=0.02).set_label("pressure")
    ax1.set_title("Pressure field")
    ax1.set_aspect("equal")

    mc = ax2.tripcolor(triang, facecolors=m, cmap="viridis")
    fig.colorbar(mc, ax=ax2, pad=0.02).set_label("local Mach")
    ax2.set_title("Mach field")
    ax2.set_aspect("equal")

    if ax_x is not None:
        ox = np.append(ax_x, ax_x[0])
        oy = np.append(ax_y, ax_y[0])
        ax1.plot(ox, oy, color="black", linewidth=1.0)
        ax2.plot(ox, oy, color="white", linewidth=1.0)

    fig.suptitle(
        f"FVM Euler (Rusanov)   C_L = {float(F['cl']):.4f}   C_D = {float(F['cd']):.4f}",
        fontsize=13,
    )
    plt.tight_layout()
    out = "results/png/fvm_solution.png"
    plt.savefig(out, dpi=150)
    print(f"Plot saved to {out}")


def main():
    plot_mesh()
    plot_solution()
    plt.show()


if __name__ == "__main__":
    main()
