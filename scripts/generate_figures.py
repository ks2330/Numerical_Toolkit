"""Regenerate the README figures from a single converged FVM solve, so the pressure
field, convergence history, mesh, and quality histograms are all mutually consistent.

    python scripts/generate_figures.py

Needs the compiled `pycfd` module (build it per DESKTOP_APP.md). Writes PNGs into docs/images/.
"""
import os
import sys

os.environ.setdefault("MPLBACKEND", "Agg")

# Make pycfd importable whether it sits next to the app or only in the MSVC build tree.
for path in ("python/cfd_studio", "build-py/bindings/Release"):
    if os.path.isdir(path):
        sys.path.insert(0, os.path.abspath(path))

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.tri as mtri
import pycfd

OUT = "docs/images"
plt.rcParams.update({"font.size": 11, "axes.titlesize": 12, "figure.dpi": 150,
                     "savefig.bbox": "tight"})

# ── converged solve: NACA 2412, M = 0.3, alpha = 2 deg ───────────────────────────
cfg = pycfd.FvmConfig()
cfg.naca = pycfd.NacaSpec(); cfg.naca.digits4 = 2412; cfg.naca.n_points = 200
cfg.density = 150.0
cfg.mach = 0.3
cfg.alpha_deg = 2.0
cfg.max_iters = 60000
cfg.tolerance = 1e-6
cfg.pressure_field_csv = ""
cfg.forces_csv = ""

print("solving…")
r = pycfd.run_fvm(cfg)
nodes = np.asarray(r.nodes)
elems = np.asarray(r.elements)
pressure = np.asarray(r.pressure, dtype=float)
res = np.asarray(r.residual_history, dtype=float)
cl, cd = r.cl, r.cd
print(f"cl={cl:.4f} cd={cd:.4f} iters={res.size} final_res={res[-1]:.2e} "
      f"watertight={r.watertight} cells={len(elems)} nodes={len(nodes)}")

triang = mtri.Triangulation(nodes[:, 0], nodes[:, 1], elems)
XLIM, YLIM = (-0.5, 1.5), (-0.6, 0.6)   # airfoil-tight window

# ── 1 · pressure field ───────────────────────────────────────────────────────────
# Area-average the per-cell pressure onto nodes for smooth (Gouraud) shading, and clip the
# colour scale to the bulk so a low-Mach field shows structure instead of washing flat.
node_p = np.zeros(len(nodes))
count = np.zeros(len(nodes))
for k in range(3):
    np.add.at(node_p, elems[:, k], pressure)
    np.add.at(count, elems[:, k], 1)
node_p /= np.maximum(count, 1)

lo, hi = np.percentile(node_p, [3.0, 97.0])
fig, ax = plt.subplots(figsize=(7.0, 4.4))
tpc = ax.tripcolor(triang, node_p, shading="gouraud", cmap="coolwarm", vmin=lo, vmax=hi)
fig.colorbar(tpc, ax=ax, label="Pressure")
ax.set_xlim(*XLIM); ax.set_ylim(*YLIM); ax.set_aspect("equal")
ax.set_title("Pressure field — NACA 2412, M = 0.3, α = 2°")
ax.text(0.02, 0.96, f"$C_L$ = {cl:.2f}\n$C_D$ = {cd:.2f}", transform=ax.transAxes,
        va="top", ha="left", bbox=dict(boxstyle="round", fc="white", ec="0.7", alpha=0.85))
ax.set_xlabel("x/c"); ax.set_ylabel("y/c")
fig.savefig(f"{OUT}/fvm_solution.png"); plt.close(fig)

# ── 2 · convergence history ──────────────────────────────────────────────────────
fig, ax = plt.subplots(figsize=(6.4, 4.4))
ax.semilogy(np.arange(res.size), res, color="tab:blue", lw=1.6)
ax.axhline(cfg.tolerance, color="tab:green", ls="--", lw=1.0, label=f"tolerance = {cfg.tolerance:g}")
ax.set_xlim(0, res.size); ax.set_xlabel("Iteration"); ax.set_ylabel("Residual (L2 norm)")
ax.set_title(f"Solver convergence — {res.size:,} iterations to {res[-1]:.1e}")
ax.grid(True, which="both", ls=":"); ax.legend(loc="upper right")
fig.savefig(f"{OUT}/fvm_solution_convergence.png"); plt.close(fig)

# ── 3 · mesh, zoomed so the near-wall clustering is visible ──────────────────────
fig, ax = plt.subplots(figsize=(6.4, 4.4))
ax.triplot(triang, color="#333333", lw=0.25)
ax.set_xlim(*XLIM); ax.set_ylim(*YLIM); ax.set_aspect("equal")
ax.set_title(f"Unstructured mesh — {len(elems):,} cells, {len(nodes):,} nodes")
ax.set_xlabel("x/c"); ax.set_ylabel("y/c")
fig.savefig(f"{OUT}/fvm_mesh.png"); plt.close(fig)

# ── 4 · mesh quality (min angle + aspect ratio) ──────────────────────────────────
p = nodes[:, :2][elems]                                   # (M, 3, 2)
side = lambda i, j: np.linalg.norm(p[:, i] - p[:, j], axis=1)
a, b, c = side(1, 2), side(2, 0), side(0, 1)              # side opposite each vertex
ang = lambda opp, s1, s2: np.degrees(np.arccos(
    np.clip((s1**2 + s2**2 - opp**2) / np.maximum(2 * s1 * s2, 1e-12), -1.0, 1.0)))
min_angle = np.min(np.stack([ang(a, b, c), ang(b, c, a), ang(c, a, b)], axis=1), axis=1)
edges = np.stack([a, b, c], axis=1)
aspect = edges.max(axis=1) / np.maximum(edges.min(axis=1), 1e-12)

fig, (axL, axR) = plt.subplots(1, 2, figsize=(10, 4))
axL.hist(min_angle, bins=np.arange(0, 61, 2), color="tab:blue", edgecolor="white", lw=0.3)
axL.axvline(30, color="tab:red", ls="--", lw=1.2, label="30° quality floor")
axL.set_xlim(0, 60)
axL.set_title(f"Smallest angle per cell — median {np.median(min_angle):.0f}° (60° is ideal)")
axL.set_xlabel("Smallest triangle angle (°)"); axL.set_ylabel("Cells")
axL.legend(loc="upper left"); axL.grid(True, ls=":")

axR.hist(np.minimum(aspect, 6.0), bins=np.arange(1, 6.05, 0.2), color="tab:green", edgecolor="white", lw=0.3)
axR.set_xlim(1, 6)
axR.set_title(f"Aspect ratio — median {np.median(aspect):.2f} (1 is equilateral)")
axR.set_xlabel("Longest / shortest side (clipped at 6)"); axR.set_ylabel("Cells")
axR.grid(True, ls=":")
fig.savefig(f"{OUT}/mesh_quality_comparison.png"); plt.close(fig)

# ── 5 · dense-vs-sparse solver benchmark (colour-blind-safe encoding) ─────────────
print("benchmarking…")
bench = pycfd.run_benchmark([5.0, 10.0, 20.0], 3, 1)
bn = np.asarray(bench.num_nodes)
bd = np.asarray(bench.dense_times)
bs = np.asarray(bench.sparse_times)
speed = bd[-1] / bs[-1]
print(f"benchmark: {speed:.0f}x at {int(bn[-1])} nodes")

fig, ax = plt.subplots(figsize=(7.2, 4.6))
# marker + linestyle + colour all differ, so the two series read in greyscale too.
ax.plot(bn, bd, "o-", color="tab:red", lw=1.8, ms=7, label="dense Gaussian  (stores all N² entries)")
ax.plot(bn, bs, "s--", color="tab:blue", lw=1.8, ms=7, label="Eigen sparse LDLT  (stores non-zeros only)")
ax.plot(bn, bd[0] * (bn / bn[0]) ** 3, ":", color="0.5", lw=1.5, label="pure N³ (dense reference)")
ax.set_yscale("log")
ax.set_xlabel("Nodes in the mesh (N)")
ax.set_ylabel("Median solve time (s), log scale")
ax.set_title("Same system, same mesh — solved two ways")
ax.annotate(f"{speed:,.0f}× faster\nat {int(bn[-1]):,} nodes",
            xy=(bn[-1], bs[-1]), xycoords="data", xytext=(0.60, 0.30), textcoords="axes fraction",
            ha="center", arrowprops=dict(arrowstyle="->", color="0.4", lw=1.2),
            bbox=dict(boxstyle="round", fc="white", ec="tab:blue", alpha=0.9))
ax.legend(loc="center left"); ax.grid(True, which="both", ls=":")
fig.savefig(f"{OUT}/benchmark_scaling.png"); plt.close(fig)

print("figures written to", OUT)
