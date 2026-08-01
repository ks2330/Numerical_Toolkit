"""
Aerofoil tabs for Numerical Toolkit Studio: FVM compressible Euler, FEM potential
flow, a surface-Cp comparison of the two, and the dense-vs-sparse solver benchmark
that times those same potential-flow solvers on aerofoil meshes.

Launched from numerical_toolkit_studio.py; the steady-heat tab lives in heat.py.
"""
import os

import pycfd
import numpy as np

from PySide6.QtWidgets import (
    QFormLayout, QHBoxLayout, QLabel, QMessageBox, QPushButton, QSizePolicy, QSplitter,
    QTabWidget, QVBoxLayout, QWidget
)
from PySide6.QtCore import QObject, QThread, Signal, Qt
import matplotlib.tri as tri

from widgets import (
    ExplainerPane, PlotPane, Problems, SummaryPane, choice, confirm_problems, controls_column, draw_mesh,
    draw_quality_histograms, int_spin, list_field, log_spin, naca_field,
    path_field, placeholder as _placeholder, section, slider_spin, spin,
)


class MeshWorker(QObject):
    finished = Signal(object)
    failed   = Signal(str)

    def __init__(self, cfg):
        super().__init__()
        self.cfg = cfg

    def run(self):
        try:
            self.finished.emit(pycfd.build_aerofoil_mesh(self.cfg))
        except Exception as e:
            self.failed.emit(str(e))


class SolverWorker(QObject):
    progress = Signal(int, float)
    finished = Signal(object)
    failed   = Signal(str)

    def __init__(self, cfg, mesh=None):
        super().__init__()
        self.cfg = cfg
        self.mesh = mesh
        self._stop = False

    def request_stop(self):
        self._stop = True

    def run(self):
        def cb(it, res):
            if it % 50 == 0:
                self.progress.emit(it, res)
            return not self._stop
        try:
            result = pycfd.run_fvm(self.cfg, cb=cb, mesh=self.mesh)
            self.finished.emit(result)
        except Exception as e:
            self.failed.emit(str(e))


class PotentialWorker(QObject):
    progress = Signal(int, float)
    finished = Signal(object)
    failed   = Signal(str)

    def __init__(self, cfg, mesh=None):
        super().__init__()
        self.cfg = cfg
        self.mesh = mesh

    def run(self):
        try:
            self.finished.emit(pycfd.run_potential(self.cfg, mesh=self.mesh))
        except Exception as e:
            self.failed.emit(str(e))


GAS_PRESETS = [
    ("Air", (1.4, 287.0)),
    ("Helium", (1.667, 2077.0)),
    ("Carbon dioxide", (1.289, 189.0)),
    ("Custom", None),
]


def _check_csv_paths(cfg, problems):
    for name, path in (("Pressure field CSV", cfg.pressure_field_csv),
                       ("Forces CSV", cfg.forces_csv)):
        if not path:
            continue
        folder = os.path.dirname(path) or "."
        if not os.path.isdir(folder):
            problems.warn(f"{name}: the folder {folder!r} does not exist, so nothing will be "
                          f"written there. Create it, pick another path, or clear the field.")
    if cfg.pressure_field_csv and cfg.pressure_field_csv == cfg.forces_csv:
        problems.error("Both CSV outputs point at the same file — one would overwrite the other.")


class GeometryFields:
    """Shared by both aerofoil panels: the bits that decide what gets meshed."""

    def __init__(self, form, density, density_low, density_high, density_tip):
        self.naca = naca_field(form, "Section:")
        self.n_points = int_spin(form, "Points:", value=160, low=40, high=600, step=20,
                                 tip="Points around the outline. More gives a smoother wall and "
                                     "a finer surface Cp, at the cost of mesh size.")
        self.density = int_spin(form, "Mesh density:", value=density, low=density_low,
                                high=density_high, step=10, tip=density_tip)

    def apply(self, cfg):
        naca = pycfd.NacaSpec()
        naca.digits4 = self.naca.digits4()
        naca.n_points = self.n_points.value()
        cfg.naca = naca
        cfg.density = float(self.density.value())

    def check(self, problems):
        self.naca.check(problems)


class InputPanel(QWidget):
    aerofoilNodes_ready = Signal(object)

    def __init__(self):
        super().__init__()
        self.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred)

        form = QFormLayout()
        form.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        form.setFieldGrowthPolicy(QFormLayout.FieldGrowthPolicy.AllNonFixedFieldsGrow)

        section(form, "Geometry")
        self.geometry = GeometryFields(
            form, density=300, density_low=20, density_high=2000,
            density_tip="Interior node budget. 300 gives roughly 10k cells and takes a few "
                        "seconds to triangulate.")
        self.algorithm = choice(
            form, "Algorithm:",
            [("Delaunay (Bowyer–Watson)", "delaunay"),
             ("Advancing Front (WIP)", "advancing_front")],
            value="delaunay", disabled=("advancing_front",),
            tip="Triangulation method. The mesher always uses constrained Delaunay; "
                "Advancing Front is on the roadmap and not yet selectable.")

        section(form, "Free stream")
        self.mach = slider_spin(form, "Mach:", value=0.3, low=0.01, high=3.0, decimals=2,
                                tip="Free-stream Mach number. The far-field boundary condition "
                                    "is written for subsonic inflow.")
        self.aoa = slider_spin(form, "AoA:", value=2.0, low=-30.0, high=30.0,
                               decimals=1, suffix=" deg",
                               tip="Applied to the flow direction, not the geometry.")
        self.rho_inf = spin(form, "Density:", value=1.0, low=1e-6, high=1e6, step=0.1, decimals=5,
                            tip="Free-stream density. With p = 1 this is a non-dimensional run.")
        self.p_inf = spin(form, "Pressure:", value=1.0, low=1e-6, high=1e9, step=0.1, decimals=5,
                          tip="Free-stream static pressure.")

        section(form, "Gas")
        self.gas = choice(form, "Preset:", GAS_PRESETS, value=(1.4, 287.0),
                          tip="Pick a gas, or Custom to set gamma and R by hand.")
        self.gamma = spin(form, "Gamma:", value=1.4, low=1.001, high=3.0, step=0.05,
                          decimals=4, tip="Cannot be 1 or below — the gas model divides by "
                                          "gamma - 1.")
        self.R = spin(form, "R:", value=287.0, low=1.0, high=1e5, step=10.0,
                      decimals=2, tip="Specific gas constant, J/(kg K).")
        self.gas.currentIndexChanged.connect(self._apply_gas_preset)
        self._apply_gas_preset()

        section(form, "Numerics")
        self.cfl = slider_spin(form, "CFL:", value=0.5, low=0.05, high=2.0, decimals=2,
                               tip="Above 1 the explicit time step is not stable.")
        self.tolerance = log_spin(form, "Tolerance:", exponent=6, low_exp=2, high_exp=14,
                                  tip="Stop when the residual L2 norm falls below this.")
        self.max_iters = int_spin(form, "Max iterations:", value=100_000, low=10,
                                  high=10_000_000, step=10_000,
                                  tip="Hard stop if the tolerance is never reached.")

        section(form, "CSV output (optional)")
        self.pressure_csv = path_field(form, "Pressure:",
                                       value="results/csv/fvm_pressure_field.csv",
                                       caption="Pressure field CSV")
        self.forces_csv = path_field(form, "Forces:", value="results/csv/fvm_forces.csv",
                                     caption="Forces CSV")

        self.setLayout(form)

    def _apply_gas_preset(self):
        preset = self.gas.currentData()
        custom = preset is None
        self.gamma.setEnabled(custom)
        self.R.setEnabled(custom)
        if not custom:
            gamma, gas_constant = preset
            self.gamma.setValue(gamma)
            self.R.setValue(gas_constant)

    def on_configure(self, problems):
        cfg = self.config(problems)
        if problems.ok():
            self.aerofoilNodes_ready.emit(pycfd.naca_outline(cfg.naca.digits4, cfg.naca.n_points))
        return cfg

    def config(self, problems):
        cfg = pycfd.FvmConfig()
        self.geometry.apply(cfg)
        cfg.mach = self.mach.value()
        cfg.alpha_deg = self.aoa.value()
        cfg.rho_inf = self.rho_inf.value()
        cfg.p_inf = self.p_inf.value()
        cfg.gamma = self.gamma.value()
        cfg.R = self.R.value()
        cfg.cfl = self.cfl.value()
        cfg.tolerance = self.tolerance.value()
        cfg.max_iters = self.max_iters.value()
        cfg.pressure_field_csv = self.pressure_csv.value()
        cfg.forces_csv = self.forces_csv.value()
        self.check(cfg, problems)
        return cfg

    def check(self, cfg, problems):
        self.geometry.check(problems)
        _check_csv_paths(cfg, problems)
        if cfg.mach >= 1.0:
            problems.warn(f"Mach {cfg.mach:g} is transonic or supersonic. The far-field boundary "
                          f"condition assumes subsonic inflow, so the answer will not be sound.")
        if cfg.cfl > 1.0:
            problems.warn(f"CFL {cfg.cfl:g} exceeds 1 — the explicit scheme will most likely "
                          f"diverge rather than converge.")
        if abs(cfg.alpha_deg) > 15.0:
            problems.warn(f"{cfg.alpha_deg:g} deg is past the stall angle of a real aerofoil. An "
                          f"inviscid solver cannot stall, so it will report lift that does not exist.")
        if cfg.density > 900:
            problems.warn(f"Mesh density {cfg.density:g} means a long triangulation and a slow "
                          f"solve. Expect minutes.")
        if cfg.tolerance < 1e-12:
            problems.warn(f"A tolerance of {cfg.tolerance:g} is at or below double-precision "
                          f"round-off; the run will almost certainly hit the iteration limit instead.")


class PotentialInputPanel(QWidget):
    """runPotential reads only naca, density and alpha_deg — so those are all this offers,
    and everything else stays at the C++ defaults."""
    aerofoilNodes_ready = Signal(object)

    def __init__(self):
        super().__init__()
        self.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred)

        form = QFormLayout()
        form.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        form.setFieldGrowthPolicy(QFormLayout.FieldGrowthPolicy.AllNonFixedFieldsGrow)

        section(form, "Geometry")
        self.geometry = GeometryFields(
            form, density=60, density_low=20, density_high=400,
            density_tip="Dense FEM solve, cost grows as N^3: 60 takes about 20 s, "
                        "150 about 50 s, 300 several minutes.")

        section(form, "Free stream")
        self.aoa = slider_spin(form, "AoA:", value=4.0, low=-30.0, high=30.0,
                               decimals=1, suffix=" deg")

        note = QLabel("Incompressible potential flow — Mach, gas properties and the time-stepping "
                      "controls do not apply.")
        note.setWordWrap(True)
        note.setStyleSheet("font-size: 11px; color: gray;")
        form.addRow(note)

        self.setLayout(form)

    def on_configure(self, problems):
        cfg = self.config(problems)
        if problems.ok():
            self.aerofoilNodes_ready.emit(pycfd.naca_outline(cfg.naca.digits4, cfg.naca.n_points))
        return cfg

    def config(self, problems):
        cfg = pycfd.FvmConfig()
        self.geometry.apply(cfg)
        cfg.alpha_deg = self.aoa.value()
        self.check(cfg, problems)
        return cfg

    def check(self, cfg, problems):
        self.geometry.check(problems)
        if cfg.density > 150:
            problems.warn(f"Mesh density {cfg.density:g} on an O(N^3) dense solve — 150 already "
                          f"takes about 50 s and it climbs steeply from there.")


def _boundary_edges(elements):
    owners = {}
    for cell, (a, b, c) in enumerate(elements.tolist()):
        for n0, n1 in ((a, b), (b, c), (c, a)):
            edge = (n0, n1) if n0 < n1 else (n1, n0)
            owners.setdefault(edge, []).append(cell)
    return {edge: cells[0] for edge, cells in owners.items() if len(cells) == 1}


def _wing_loop(nodes, elements):
    boundary = _boundary_edges(elements)
    neighbours = {}
    for edge in boundary:
        neighbours.setdefault(edge[0], []).append(edge)
        neighbours.setdefault(edge[1], []).append(edge)

    loops, seen = [], set()
    for start in boundary:
        if start in seen:
            continue
        seen.add(start)
        stack, loop = [start], []
        while stack:
            edge = stack.pop()
            loop.append(edge)
            for node in edge:
                for neighbour in neighbours[node]:
                    if neighbour not in seen:
                        seen.add(neighbour)
                        stack.append(neighbour)
        loops.append(loop)

    if len(loops) < 2:
        return None, None

    def bbox_area(loop):
        points = nodes[np.unique(np.asarray(loop))]
        return float(np.ptp(points[:, 0]) * np.ptp(points[:, 1]))

    return min(loops, key=bbox_area), boundary


def wing_loop_nodes(nodes, elements):
    loop, _ = _wing_loop(nodes, elements)
    return None if loop is None else np.unique(np.asarray(loop))


def _chord_frame(ring):
    leading = ring[np.argmin(ring[:, 0])]
    trailing = ring[np.argmax(ring[:, 0])]
    chord = trailing - leading
    length2 = float(chord @ chord)
    return (leading, chord, length2) if length2 > 0.0 else None


def _project(points, frame):
    leading, chord, length2 = frame
    offset = points - leading
    x_over_c = (offset @ chord) / length2
    upper = chord[0] * offset[:, 1] - chord[1] * offset[:, 0] > 0
    return x_over_c, upper


def _focus_on_aerofoil(axes, nodes, elements):
    ids = wing_loop_nodes(nodes, elements)
    if ids is None:
        return
    ring = nodes[ids]
    x0, x1 = ring[:, 0].min(), ring[:, 0].max()
    centre = 0.5 * (ring[:, 1].min() + ring[:, 1].max())
    chord = max(x1 - x0, 1e-9)
    axes.set_xlim(x0 - 0.5 * chord, x1 + 1.0 * chord)
    axes.set_ylim(centre - 0.75 * chord, centre + 0.75 * chord)


def fem_surface_cp(result, cfg):
    ids = wing_loop_nodes(result.nodes, result.elements)
    if ids is None:
        return None
    ring = result.nodes[ids]
    frame = _chord_frame(ring)
    if frame is None:
        return None
    x_over_c, upper = _project(ring, frame)
    return x_over_c, np.asarray(result.field)[ids], upper


def fvm_surface_cp(result, cfg):
    if cfg is None:
        return None
    q_inf = 0.5 * cfg.gamma * cfg.p_inf * cfg.mach ** 2      # identical to the solver's own qInf
    if q_inf <= 0.0:
        return None
    loop, boundary = _wing_loop(result.nodes, result.elements)
    if loop is None:
        return None
    edges = np.asarray(loop)
    cells = np.array([boundary[edge] for edge in loop])
    frame = _chord_frame(result.nodes[np.unique(edges)])
    if frame is None:
        return None
    midpoints = 0.5 * (result.nodes[edges[:, 0]] + result.nodes[edges[:, 1]])
    x_over_c, upper = _project(midpoints, frame)
    return x_over_c, (result.pressure[cells] - cfg.p_inf) / q_inf, upper


def _render_field(axes, result, values, cmap, title, cbar_label):
    triangulated_mesh = tri.Triangulation(result.nodes[:, 0], result.nodes[:, 1], result.elements)
    values = np.asarray(values, dtype=float)
    # Clip the colour scale to the bulk of the field — a few stagnation cells would otherwise
    # saturate the map and wash a low-Mach field out to a flat colour.
    low, high = np.percentile(values, [2.0, 98.0])
    mappable = axes.tripcolor(triangulated_mesh, facecolors=values, cmap=cmap, vmin=low, vmax=high)
    axes.figure.colorbar(mappable, ax=axes, label=cbar_label)
    axes.set_title(title)
    axes.set_xlabel("X")
    axes.set_ylabel("Y")
    axes.set_aspect("equal")
    _focus_on_aerofoil(axes, result.nodes, result.elements)


def render_pressure(axes, result, cfg):
    _render_field(axes, result, result.pressure, "coolwarm",
                  f"Pressure field — c_l = {result.cl:.3e}, c_d = {result.cd:.3e}", "Pressure")


def render_mach(axes, result, cfg):
    _render_field(axes, result, result.mach, "viridis", "Mach number field", "Mach")


def render_residual_history(axes, result, cfg):
    residuals = np.asarray(result.residual_history, dtype=float)
    axes.set_title(f"Residual history — {residuals.size} iterations")
    axes.set_xlabel("Iteration")
    axes.set_ylabel("Residual (L2 norm)")
    axes.grid(True, which="both", ls=":")
    if not np.any(residuals > 0):
        _placeholder(axes, "no positive residuals recorded")
        return
    axes.set_yscale("log")
    axes.plot(np.arange(residuals.size), residuals, lw=1.2, color="blue")
    if cfg is not None and cfg.tolerance > 0:
        axes.axhline(cfg.tolerance, color="tab:green", ls="--", lw=1.0, label="tolerance")
        axes.legend(loc="best")


def render_fem_cp(axes, result, cfg):
    field = np.asarray(result.field)
    triangulated_mesh = tri.Triangulation(result.nodes[:, 0], result.nodes[:, 1], result.elements)
    low, high = np.percentile(field, [1.0, 99.0])
    mappable = axes.tripcolor(triangulated_mesh, field, shading="gouraud", cmap="coolwarm",
                              vmin=low, vmax=high)
    axes.figure.colorbar(mappable, ax=axes, label="$C_p$")
    axes.set_title(f"Potential-flow $C_p$ field (scale clipped to {low:.2f} … {high:.2f})")
    axes.set_xlabel("X")
    axes.set_ylabel("Y")
    axes.set_aspect("equal")
    _focus_on_aerofoil(axes, result.nodes, result.elements)


def _draw_surface_cp(axes, curve, title):
    axes.set_title(title)
    axes.set_xlabel("x / c")
    axes.set_ylabel("$C_p$")
    axes.grid(True, ls=":")
    if curve is None:
        _placeholder(axes, "no aerofoil boundary found in this mesh")
        return
    x_over_c, cp, upper = curve
    for mask, label, colour in ((upper, "upper surface", "tab:red"),
                                (~upper, "lower surface", "tab:blue")):
        if not mask.any():
            continue
        order = np.argsort(x_over_c[mask])
        axes.plot(x_over_c[mask][order], cp[mask][order], lw=1.4, color=colour,
                  marker=".", markersize=3, label=label)
    axes.axhline(0.0, color="grey", lw=0.8)
    axes.invert_yaxis()
    axes.legend(loc="best")


def render_fvm_cp_surface(axes, result, cfg):
    _draw_surface_cp(axes, fvm_surface_cp(result, cfg), "Surface pressure coefficient")


def render_fem_cp_surface(axes, result, cfg):
    _draw_surface_cp(axes, fem_surface_cp(result, cfg), "Surface pressure coefficient")


def fvm_summary(result, cfg):
    residuals = result.residual_history
    return [
        ("Lift coefficient c_l", f"{result.cl:.6f}"),
        ("Drag coefficient c_d", f"{result.cd:.6f}"),
        ("Lift / drag", f"{result.cl / result.cd:.3f}" if result.cd else "n/a"),
        ("Lift", f"{result.lift:.6e}"),
        ("Drag", f"{result.drag:.6e}"),
        ("Force x", f"{result.fx:.6e}"),
        ("Force y", f"{result.fy:.6e}"),
        ("Watertight mesh", "yes" if result.watertight else "no"),
        ("Cells", f"{len(result.elements)}"),
        ("Nodes", f"{len(result.nodes)}"),
        ("Iterations", f"{len(residuals)}"),
        ("Final residual", f"{residuals[-1]:.3e}" if len(residuals) else "n/a"),
    ]


def fem_summary(result, cfg):
    field = np.asarray(result.field)
    rows = [
        ("Nodes", f"{len(result.nodes)}"),
        ("Cells", f"{len(result.elements)}"),
        ("C_p min (suction peak)", f"{field.min():.4f}"),
        ("C_p max (stagnation)", f"{field.max():.4f}"),
        ("Stagnation error vs 1.0", f"{abs(1.0 - field.max()):.4f}"),
        ("C_p mean", f"{field.mean():.4f}"),
    ]
    ids = wing_loop_nodes(result.nodes, result.elements)
    if ids is not None:
        wall = field[ids]
        rows += [
            ("Wall nodes", f"{len(ids)}"),
            ("Wall C_p min", f"{wall.min():.4f}"),
            ("Wall C_p max", f"{wall.max():.4f}"),
        ]
    return rows


class SolverSpec:
    def __init__(self, label, worker_class, result_plots, summary, surface_cp,
                 iterative, stoppable):
        self.label = label
        self.worker_class = worker_class
        self.result_plots = result_plots
        self.summary = summary
        self.surface_cp = surface_cp
        self.iterative = iterative
        self.stoppable = stoppable


FVM_SPEC = SolverSpec(
    "FVM Euler", SolverWorker,
    [("Pressure", render_pressure), ("Mach", render_mach),
     ("Cp surface", render_fvm_cp_surface), ("Residuals", render_residual_history)],
    fvm_summary, fvm_surface_cp, iterative=True, stoppable=True,
)

FEM_SPEC = SolverSpec(
    "Potential flow", PotentialWorker,
    [("Cp field", render_fem_cp), ("Cp surface", render_fem_cp_surface)],
    fem_summary, fem_surface_cp, iterative=False, stoppable=False,
)


class PlotPanel(QTabWidget):
    def __init__(self, spec, parent=None):
        super().__init__(parent)
        self.setTabPosition(QTabWidget.TabPosition.South)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)

        self.spec = spec
        self.result = None
        self.cfg = None

        self.aerofoil_pane = PlotPane()
        self.mesh_pane = PlotPane()
        self.quality_pane = PlotPane()
        self.addTab(self.aerofoil_pane, "Aerofoil")
        self.addTab(self.mesh_pane, "Mesh")
        self.addTab(self.quality_pane, "Mesh quality")
        self.aerofoil_pane.show_placeholder("press Configure")
        self.mesh_pane.show_placeholder("press Create and Plot Mesh")
        self.quality_pane.show_placeholder("press Create and Plot Mesh")

        self.convergence_pane = None
        if spec.iterative:
            self.convergence_pane = PlotPane()
            self.addTab(self.convergence_pane, "Convergence")

        self.result_panes = []
        for label, renderer in spec.result_plots:
            pane = PlotPane(renderer)
            pane.show_placeholder("run a simulation")
            self.addTab(pane, label)
            self.result_panes.append(pane)

        self.summary_pane = SummaryPane(spec.summary)
        self.addTab(self.summary_pane, "Summary")
        self.result_panes.append(self.summary_pane)

        self.it_data = []
        self.res_data = []
        self.conv_line = None
        self.reset_convergence()

        self.currentChanged.connect(self._render_current)

    def set_config(self, cfg):
        self.cfg = cfg

    def _render_current(self, _index=None):
        pane = self.currentWidget()
        if pane is self.convergence_pane:
            self._refresh_convergence()
        elif getattr(pane, "dirty", False) and self.result is not None:
            pane.render(self.result, self.cfg)

    def show_busy(self, message):
        pane = self.result_panes[0]
        pane.show_placeholder(message)
        pane.dirty = True
        self.setCurrentWidget(pane)

    def reset_convergence(self):
        if self.convergence_pane is None:
            return
        self.it_data = []
        self.res_data = []
        axes = self.convergence_pane.reset_axes()
        (self.conv_line,) = axes.plot([], [], lw=2, color="blue")
        axes.set_yscale("log")
        axes.set_title("Solver convergence")
        axes.set_xlabel("Iteration")
        axes.set_ylabel("Residual (L2 norm)")
        axes.grid(True, which="both", ls=":")
        self.convergence_pane.canvas.draw_idle()

    def plot_iteration(self, it, res):
        if self.convergence_pane is None:
            return
        if it == 0:
            self.reset_convergence()
        self.it_data.append(it)
        self.res_data.append(res)
        if self.currentWidget() is self.convergence_pane:
            self._refresh_convergence()

    def _refresh_convergence(self):
        if self.convergence_pane is None or not self.it_data:
            return
        axes = self.convergence_pane.axes
        self.conv_line.set_data(self.it_data, self.res_data)
        axes.set_title(f"Solver convergence — iter {self.it_data[-1]}, residual {self.res_data[-1]:.2e}")
        axes.set_xlim(0, max(self.it_data[-1], 1))
        positive = [r for r in self.res_data if r > 0]
        if positive:
            axes.set_ylim(min(positive) * 0.7, max(positive) * 1.5)
        self.convergence_pane.canvas.draw_idle()

    def plot_aerofoil(self, mesh_data):
        axes = self.aerofoil_pane.reset_axes()
        axes.plot(mesh_data[:, 0], mesh_data[:, 1], color="red", linewidth=1.8, label="aerofoil surface")
        axes.set_title("Aerofoil outline")
        axes.set_xlabel("X")
        axes.set_ylabel("Y")
        axes.set_aspect("equal")
        axes.grid(True)
        axes.legend(loc="upper right")
        self.aerofoil_pane.canvas.draw_idle()
        self.setCurrentWidget(self.aerofoil_pane)

    def plot_mesh(self, nodes, elements, switch=True):
        draw_mesh(self.mesh_pane.reset_axes(), nodes, elements)
        self.mesh_pane.canvas.draw_idle()
        self.quality_pane.axes = draw_quality_histograms(
            self.quality_pane.reset_figure(), nodes, elements)
        self.quality_pane.canvas.draw_idle()
        if switch:
            self.setCurrentWidget(self.mesh_pane)

    def plot_solution(self, result):
        self.result = result
        self.plot_mesh(result.nodes, result.elements, switch=False)   # Run without Mesh still fills those tabs
        for pane in self.result_panes:
            pane.dirty = True
        self.setCurrentWidget(self.result_panes[0])
        self._render_current()


class ComparePanel(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.curves = {}
        self.pane = PlotPane()
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.pane)
        self._draw()

    def add(self, spec, result, cfg):
        curve = spec.surface_cp(result, cfg)
        if curve is not None:
            self.curves[spec.label] = curve
        self._draw()

    def _draw(self):
        axes = self.pane.reset_axes()
        if not self.curves:
            _placeholder(axes, "run a solver on each tab to compare their surface C_p")
            self.pane.canvas.draw_idle()
            return

        axes.set_title("Surface $C_p$ — solver comparison")
        axes.set_xlabel("x / c")
        axes.set_ylabel("$C_p$")
        axes.grid(True, ls=":")
        colours = ["tab:red", "tab:blue", "tab:green", "tab:purple"]
        for (label, (x_over_c, cp, upper)), colour in zip(sorted(self.curves.items()), colours):
            for mask, style, side in ((upper, "-", "upper"), (~upper, "--", "lower")):
                if not mask.any():
                    continue
                order = np.argsort(x_over_c[mask])
                axes.plot(x_over_c[mask][order], cp[mask][order], style, lw=1.3, color=colour,
                          label=f"{label} — {side}")
        axes.axhline(0.0, color="grey", lw=0.8)
        axes.invert_yaxis()
        axes.legend(loc="best", fontsize=8)
        self.pane.canvas.draw_idle()


class SolverTab(QWidget):
    result_ready = Signal(object, object, object)

    def __init__(self, input_panel_class, spec):
        super().__init__()

        self.spec = spec
        self.input_panel = input_panel_class()
        self.plot_panel = PlotPanel(spec)
        self.mesh = None
        self.result = None
        self.thread = None
        self.worker = None
        self._cfg = None
        self._running = False
        self._stop_requested = False

        self.configure_btn = QPushButton("Configure")
        self.meshify = QPushButton("Create and Plot Mesh")
        self.calc = QPushButton("Run Simulation")
        self.stop_btn = QPushButton("Stop")
        self.stop_btn.setEnabled(False)
        if not spec.stoppable:
            self.stop_btn.setToolTip("run_potential has no progress callback, so it cannot be interrupted")

        self.status = QLabel("Ready.")
        self.status.setWordWrap(True)
        self.status.setStyleSheet("color: gray; font-size: 12px;")

        run_row = QHBoxLayout()
        run_row.addWidget(self.calc)
        run_row.addWidget(self.stop_btn)

        controls = QVBoxLayout()
        controls.addWidget(self.input_panel)
        controls.addWidget(self.configure_btn)
        controls.addWidget(self.meshify)
        controls.addLayout(run_row)
        controls.addWidget(self.status)
        controls.addStretch()

        controls_widget = controls_column(controls)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(controls_widget)
        splitter.addWidget(self.plot_panel)
        splitter.setStretchFactor(0, 0)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([430, 1070])

        main_layout = QHBoxLayout(self)
        main_layout.addWidget(splitter)

        self.configure_btn.clicked.connect(self.show_outline)
        self.meshify.clicked.connect(self.start_mesh)
        self.calc.clicked.connect(self.start_solve)
        self.stop_btn.clicked.connect(self.stop_solve)

        self.input_panel.aerofoilNodes_ready.connect(self.plot_panel.plot_aerofoil)

    def show_outline(self):
        problems = Problems()
        self.input_panel.on_configure(problems)
        if problems.errors:
            confirm_problems(self, self.spec.label, problems)
            return
        self.status.setText("Aerofoil outline generated.")

    def start_mesh(self):
        cfg = self._config_or_warn()
        if cfg is None:
            return
        self._set_busy(True, "Building mesh…")
        self._start(MeshWorker(cfg), {"finished": self.on_mesh_ready})

    def start_solve(self):
        cfg = self._config_or_warn()
        if cfg is None:
            return

        self._cfg = cfg
        self._stop_requested = False
        self.plot_panel.set_config(cfg)

        mesh, self.mesh = self.mesh, None
        if self.spec.iterative:
            self.plot_panel.reset_convergence()
            self.plot_panel.setCurrentWidget(self.plot_panel.convergence_pane)
        else:
            nodes = len(mesh.nodes) if mesh is not None else "?"
            self.plot_panel.show_busy(f"{self.spec.label}: direct FEM solve running…\n"
                                      f"{nodes} nodes, cost grows about N^3")

        self._set_busy(True, "Solving…" if mesh is not None else "Meshing, then solving…",
                       can_stop=self.spec.stoppable)
        self._start(self.spec.worker_class(cfg, mesh),
                    {"finished": self.on_result, "progress": self.on_progress})

    def stop_solve(self):
        if self._running and hasattr(self.worker, "request_stop"):
            self._stop_requested = True
            self.worker.request_stop()
            self.stop_btn.setEnabled(False)
            self.status.setText("Stopping at the next iteration…")

    def _start(self, worker, connections):
        self.thread = QThread()
        self.worker = worker
        worker.moveToThread(self.thread)
        self.thread.started.connect(worker.run)

        for signal_name, slot in connections.items():
            getattr(worker, signal_name).connect(slot)
        if isinstance(worker, (SolverWorker, PotentialWorker)):
            worker.progress.connect(self.plot_panel.plot_iteration)
        worker.failed.connect(self.on_failed)

        worker.finished.connect(self.thread.quit)
        worker.failed.connect(self.thread.quit)
        worker.finished.connect(worker.deleteLater)
        worker.failed.connect(worker.deleteLater)
        self.thread.finished.connect(self.thread.deleteLater)
        self.thread.start()

    def on_mesh_ready(self, mesh):
        self.mesh = mesh
        self.plot_panel.plot_mesh(mesh.nodes, mesh.elements)
        self._set_busy(False, f"Mesh ready — {len(mesh.elements)} cells, {len(mesh.nodes)} nodes.")

    def on_progress(self, it, res):
        self.status.setText(f"Solving — iteration {it}, residual {res:.3e}")

    def on_result(self, result):
        self.result = result
        self.plot_panel.plot_solution(result)
        self.result_ready.emit(self.spec, result, self._cfg)

        if not self.spec.iterative:
            self._set_busy(False, f"{self.spec.label} solve complete — {len(result.nodes)} nodes.")
            return

        residuals = result.residual_history
        iterations = len(residuals)
        final = float(residuals[-1]) if iterations else float("nan")
        if self._stop_requested:
            verb = "stopped"
        elif self._cfg is not None and final < self._cfg.tolerance:
            verb = "converged"
        else:
            verb = "hit the iteration limit"
        self._set_busy(False, f"Run {verb} after {iterations} iterations (residual {final:.3e}) — "
                              f"c_l = {result.cl:.4f}, c_d = {result.cd:.4f}")

    def on_failed(self, message):
        self._set_busy(False, "Failed.")
        self._report(message)

    def _config_or_warn(self):
        if self._running:
            return None
        problems = Problems()
        cfg = self.input_panel.config(problems)
        return cfg if confirm_problems(self, self.spec.label, problems) else None

    def _set_busy(self, running, message, can_stop=False):
        self._running = running
        for button in (self.configure_btn, self.meshify, self.calc):
            button.setEnabled(not running)
        self.stop_btn.setEnabled(running and can_stop)
        self.status.setText(message)

    def _report(self, message):
        self.status.setText(message)
        QMessageBox.warning(self, f"{self.spec.label} — problem", message)


class BenchmarkWorker(QObject):
    finished = Signal(object)
    failed   = Signal(str)

    def __init__(self, densities, reps, warmup):
        super().__init__()
        self.densities = densities
        self.reps = reps
        self.warmup = warmup

    def run(self):
        try:
            self.finished.emit(pycfd.run_benchmark(self.densities, self.reps, self.warmup))
        except Exception as e:
            self.failed.emit(str(e))


class BenchmarkInputPanel(QWidget):
    def __init__(self):
        super().__init__()
        self.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred)
        form = QFormLayout()
        form.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        form.setFieldGrowthPolicy(QFormLayout.FieldGrowthPolicy.AllNonFixedFieldsGrow)

        self.densities = list_field(
            form, "Densities:", value="5, 10, 20", low=2, high=400,
            tip="Comma-separated aerofoil mesh densities — the same knob the Potential Flow tab\n"
                "uses. Each builds a mesh and becomes one point on the charts. Node count grows\n"
                "slowly (5 gives ~2600 nodes, 20 gives ~3000) but dense solve time grows as N^3.")
        self.reps = int_spin(form, "Reps:", value=3, low=1, high=25,
                             tip="Timed repeats per solver per density. The median is reported.")
        self.warmup = int_spin(form, "Warm-up:", value=1, low=0, high=10,
                               tip="Untimed runs before timing starts, to warm caches and allocators.")
        self.setLayout(form)

    def values(self, problems):
        densities = self.densities.values(problems)
        reps = self.reps.value()
        warmup = self.warmup.value()
        if densities:
            self.check(densities, reps, warmup, problems)
        return densities, reps, warmup

    def check(self, densities, reps, warmup, problems):
        if len(set(densities)) != len(densities):
            problems.warn("Densities repeats a value — you will benchmark the same mesh twice.")
        if len(densities) < 2:
            problems.warn("With one density there is nothing to plot a trend against; the "
                          "scaling exponents cannot be fitted.")
        elif max(densities) / min(densities) < 2.0:
            problems.warn(f"Densities only span {max(densities) / min(densities):.1f}x. Node "
                          f"count will barely move, so the fitted N^k exponents will be noise.")
        dense_solves = len(densities) * (reps + warmup)
        if dense_solves * 5 > 180:
            problems.warn(f"About {dense_solves} dense solves at roughly 5 s each — budget "
                          f"{dense_solves * 5 // 60} minutes or more. The window stays responsive.")


BENCHMARK_EXPLAINER = """\
<h3>What this benchmark measures</h3>
<p>This times the <b>same potential-flow solve the Potential Flow tab runs</b>, on the same
kind of aerofoil mesh. Both solvers are handed the <b>identical</b> problem on the
<b>identical</b> mesh (fixed seed, so it is reproducible) and asked for the same answer:
the velocity potential <i>phi</i> at every node. The only thing that differs is how the
linear system <i>K phi = b</i> gets solved.</p>

<table cellpadding="4">
<tr><td><b>Dense Gaussian</b></td>
    <td><tt>DefaultPotentialFlowSolver</tt> — the original implementation, and what the
    Potential Flow tab still uses. Stores the whole N x N stiffness matrix and runs
    textbook Gaussian elimination. Work grows as <b>N&sup3;</b>, memory as N&sup2;.</td></tr>
<tr><td><b>Eigen sparse LDLT</b></td>
    <td><tt>EigenSparsePotentialFlowSolver</tt> — the optimisation. K is almost entirely
    zeros (a node only couples to its mesh neighbours), so it stores only the non-zeros
    and factorises with <tt>SimplicialLDLT</tt>.</td></tr>
</table>

<h3>Reading the tabs</h3>
<ul>
<li><b>Times</b> — median solve time against node count, log scale. The dashed line is a
pure N&sup3; curve through the first dense point: if dense tracks it, the cubic cost is
confirmed. Sparse should look almost flat by comparison.</li>
<li><b>Speedup</b> — dense time divided by sparse time. This is the headline number.</li>
<li><b>Agreement</b> — the largest difference between the two solvers' answers for phi,
node by node. <b>This is the tab that makes the speedup meaningful:</b> a fast solver that
returns a different answer is worthless. Expect ~1e-12, i.e. floating-point round-off.</li>
<li><b>Table</b> — the same numbers in full, plus the fitted cost exponents.</li>
</ul>

<h3>Inputs</h3>
<p><b>Densities</b> is a list of aerofoil mesh densities — one benchmark point each.
<b>Reps</b> are timed runs per solver per density (the median is reported);
<b>Warm-up</b> runs are untimed and just prime caches.</p>

<p><i>Total work is</i> <tt>len(densities) x (reps + warmup) x 2</tt> <i>solves. Each dense
solve at these sizes takes several seconds, so the whole run is typically minutes.</i></p>

<p><b>Note on the exponents:</b> the default densities only span ~2600 to ~3000 nodes.
That 16% range is far too narrow to fit a power law, so treat the reported N^k as
indicative only. Widen the densities so node count spans a factor of several if you want
the N&sup3; claim to actually hold up.</p>
"""


def render_bench_times(axes, result, cfg):
    nodes = np.asarray(result.num_nodes, dtype=float)
    dense = np.asarray(result.dense_times)
    sparse = np.asarray(result.sparse_times)

    axes.plot(nodes, dense, "o-", color="tab:red", label="dense Gaussian  (stores all N² entries)")
    axes.plot(nodes, sparse, "s--", color="tab:blue", label="Eigen sparse LDLT  (stores non-zeros only)")
    if len(nodes) > 1:
        reference = dense[0] * (nodes / nodes[0]) ** 3
        axes.plot(nodes, reference, "--", color="grey", lw=1.2,
                  label="pure N³ through the first dense point")

    axes.set_yscale("log")
    axes.set_title("Same system, same mesh — solved two ways", fontsize=11)
    axes.set_xlabel("Nodes in the mesh (N)")
    axes.set_ylabel("Median time for one solve (s), log scale")
    axes.grid(True, which="both", ls=":")
    axes.legend(loc="center left", fontsize=8)
    axes.annotate(f"{dense[-1] / sparse[-1]:,.0f}x apart\nat {int(nodes[-1]):,} nodes",
                  xy=(0.78, 0.5), xycoords="axes fraction", ha="center", va="center",
                  fontsize=10, color="tab:purple",
                  bbox=dict(boxstyle="round,pad=0.4", fc="white", ec="tab:purple", alpha=0.85))


def render_bench_speedup(axes, result, cfg):
    nodes = np.asarray(result.num_nodes)
    speedups = np.asarray(result.speedups)
    axes.bar([f"{n:,}" for n in nodes], speedups, color="tab:purple")
    for index, value in enumerate(speedups):
        axes.text(index, value, f"{value:,.0f}x", ha="center", va="bottom", fontsize=9)
    axes.set_title("How many times faster the sparse solver is", fontsize=11)
    axes.set_xlabel("Nodes in the mesh (N)")
    axes.set_ylabel("Dense time ÷ sparse time")
    axes.margins(y=0.15)
    axes.grid(True, axis="y", ls=":")


def render_bench_agreement(axes, result, cfg):
    nodes = np.asarray(result.num_nodes)
    diffs = np.asarray(result.max_diffs)
    axes.semilogy(nodes, np.maximum(diffs, 1e-18), "o-", color="tab:blue",
                  label="largest node-wise difference in phi")
    axes.axhline(1e-9, color="tab:red", ls="--", lw=1.0, label="1e-9 — anything below is round-off")
    axes.set_title("Do the two solvers agree?", fontsize=11)
    axes.set_xlabel("Nodes in the mesh (N)")
    axes.set_ylabel("max |phi_sparse − phi_dense|")
    axes.grid(True, which="both", ls=":")
    axes.legend(loc="best", fontsize=8)
    verdict = "identical to round-off" if diffs.max() < 1e-9 else "DISAGREE — speedup is not valid"
    axes.annotate(verdict, xy=(0.5, 0.06), xycoords="axes fraction", ha="center", fontsize=10,
                  color="tab:green" if diffs.max() < 1e-9 else "tab:red")


def benchmark_summary(result, cfg):
    nodes = np.asarray(result.num_nodes)
    dense = np.asarray(result.dense_times)
    sparse = np.asarray(result.sparse_times)
    speedups = np.asarray(result.speedups)
    diffs = np.asarray(result.max_diffs)

    rows = [
        ("Same mesh, same system, solved two ways. 'max diff' is how far apart the answers are.", ""),
        ("", ""),
        ("Nodes      dense (s)   sparse (s)    speedup     max diff", ""),
    ]
    for n, d, s, up, diff in zip(nodes, dense, sparse, speedups, diffs):
        rows.append((f"{n:<10d} {d:<11.4f} {s:<13.6f} {up:<11,.0f} {diff:.2e}", ""))

    rows += [("", ""), (f"Best speedup: {speedups.max():,.0f}x at {nodes[speedups.argmax()]} nodes", "")]
    if len(nodes) > 1:
        span = nodes[-1] / nodes[0]
        growth = np.log(dense[-1] / dense[0]) / np.log(span)
        sparse_growth = np.log(max(sparse[-1] / sparse[0], 1e-12)) / np.log(span)
        rows.append((f"Dense cost scales as N^{growth:.2f}  (N^3 is the textbook expectation)", ""))
        rows.append((f"Sparse cost scales as N^{sparse_growth:.2f}", ""))
        if span < 2.0:
            rows.append((f"  ^ node count only spans {span:.2f}x — too narrow to fit a power law, "
                         f"treat these as indicative", ""))
    rows += [
        ("", ""),
        (f"Worst disagreement: {diffs.max():.2e}", ""),
        ("Verdict: " + ("answers match to round-off, so the speedup is real."
                        if diffs.max() < 1e-9 else
                        "answers DIFFER — investigate before trusting the speedup."), ""),
    ]
    return rows


BENCH_PLOTS = [
    ("Times", render_bench_times),
    ("Speedup", render_bench_speedup),
    ("Agreement", render_bench_agreement),
]


class BenchmarkTab(QWidget):
    def __init__(self):
        super().__init__()
        self.input_panel = BenchmarkInputPanel()
        self.result = None
        self.thread = None
        self.worker = None
        self._running = False

        self.panel = QTabWidget()
        self.panel.setTabPosition(QTabWidget.TabPosition.South)
        self.panel.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)

        self.explainer = ExplainerPane(BENCHMARK_EXPLAINER)
        self.panel.addTab(self.explainer, "What is this?")

        self.panes = []
        for label, renderer in BENCH_PLOTS:
            pane = PlotPane(renderer)
            pane.show_placeholder("press Run Benchmark")
            self.panel.addTab(pane, label)
            self.panes.append(pane)
        self.summary_pane = SummaryPane(benchmark_summary)
        self.panel.addTab(self.summary_pane, "Table")
        self.panes.append(self.summary_pane)
        self.panel.currentChanged.connect(self._render_current)

        self.run_btn = QPushButton("Run Benchmark")
        self.blurb = QLabel(
            "Times the <b>same</b> potential-flow solve as the Potential Flow tab — once with "
            "the original dense Gaussian solver (O(N³)), once with Eigen's sparse LDLT — and "
            "checks the answers match. See the <b>What is this?</b> tab.")
        self.blurb.setWordWrap(True)
        self.blurb.setTextFormat(Qt.TextFormat.RichText)
        self.blurb.setStyleSheet("font-size: 12px;")

        self.status = QLabel("Ready.")
        self.status.setWordWrap(True)
        self.status.setStyleSheet("color: gray; font-size: 12px;")

        controls = QVBoxLayout()
        controls.addWidget(self.input_panel)
        controls.addWidget(self.run_btn)
        controls.addWidget(self.blurb)
        controls.addWidget(self.status)
        controls.addStretch()

        self.input_panel.densities.edit.textChanged.connect(self._update_cost)
        self.input_panel.reps.valueChanged.connect(self._update_cost)
        self.input_panel.warmup.valueChanged.connect(self._update_cost)
        self._update_cost()

        controls_widget = controls_column(controls)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(controls_widget)
        splitter.addWidget(self.panel)
        splitter.setStretchFactor(0, 0)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([430, 1070])

        layout = QHBoxLayout(self)
        layout.addWidget(splitter)

        self.run_btn.clicked.connect(self.start_benchmark)

    def _render_current(self, _index=None):
        pane = self.panel.currentWidget()
        if getattr(pane, "dirty", False) and self.result is not None:
            pane.render(self.result, None)

    def _update_cost(self, _value=None):
        if self._running:
            return
        problems = Problems()
        densities, reps, warmup = self.input_panel.values(problems)
        if problems.errors:
            self.status.setText("Densities: " + problems.errors[0])
            return
        solves = len(densities) * (reps + warmup) * 2
        self.status.setText(f"{len(densities)} mesh sizes x ({reps} timed + {warmup} warm-up) "
                            f"x 2 solvers = {solves} solves. Dense solves dominate — "
                            f"budget roughly {solves // 2 * 5}s.")

    def start_benchmark(self):
        if self._running:
            return
        problems = Problems()
        densities, reps, warmup = self.input_panel.values(problems)
        if not confirm_problems(self, "Solver benchmark", problems):
            return

        runs = len(densities) * (reps + warmup) * 2
        self._set_busy(True, f"Running {runs} solves across {len(densities)} mesh sizes…")
        self.panes[0].show_placeholder(f"benchmarking {len(densities)} mesh sizes…")
        self.panel.setCurrentWidget(self.panes[0])

        self.thread = QThread()
        self.worker = BenchmarkWorker(densities, reps, warmup)
        self.worker.moveToThread(self.thread)
        self.thread.started.connect(self.worker.run)
        self.worker.finished.connect(self.on_result)
        self.worker.failed.connect(self.on_failed)
        self.worker.finished.connect(self.thread.quit)
        self.worker.failed.connect(self.thread.quit)
        self.worker.finished.connect(self.worker.deleteLater)
        self.worker.failed.connect(self.worker.deleteLater)
        self.thread.finished.connect(self.thread.deleteLater)
        self.thread.start()

    def on_result(self, result):
        self.result = result
        for pane in self.panes:
            pane.dirty = True
        self.panel.setCurrentWidget(self.panes[0])
        self._render_current()
        speedups = np.asarray(result.speedups)
        diffs = np.asarray(result.max_diffs)
        self._set_busy(False, f"Done — sparse is {speedups.min():,.0f}x to {speedups.max():,.0f}x "
                              f"faster, agreeing to {diffs.max():.1e}")

    def on_failed(self, message):
        self._set_busy(False, "Failed.")
        self._report(message)

    def _set_busy(self, running, message):
        self._running = running
        self.run_btn.setEnabled(not running)
        self.status.setText(message)

    def _report(self, message):
        self.status.setText(message)
        QMessageBox.warning(self, "Benchmark — problem", message)


def build_tabs():
    """(widget, title) for each aerofoil tab, already wired to the shared Compare panel."""
    compare = ComparePanel()
    fvm = SolverTab(InputPanel, FVM_SPEC)
    potential = SolverTab(PotentialInputPanel, FEM_SPEC)
    for tab in (fvm, potential):
        tab.result_ready.connect(compare.add)
    return [
        (fvm, "FVM Euler"),
        (potential, "Potential Flow"),
        (compare, "Compare Cp"),
        (BenchmarkTab(), "Solver Benchmark"),
    ]
