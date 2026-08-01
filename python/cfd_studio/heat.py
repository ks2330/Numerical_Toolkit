"""
Steady-heat tab for Numerical Toolkit Studio.

`run_heat` solves Laplace on a width x height rectangle with the left edge held at
T_inlet, the right edge at 0 and top/bottom insulated, so the exact answer is the
linear ramp T(x) = T_inlet * (1 - x/width). Every plot here is scored against it.

Launched from numerical_toolkit_studio.py; the aerofoil tabs live in aerofoil.py.
"""
import pycfd
import numpy as np

from PySide6.QtWidgets import (
    QFormLayout, QHBoxLayout, QLabel, QMessageBox, QPushButton, QSizePolicy, QSplitter,
    QTabWidget, QVBoxLayout, QWidget
)
from PySide6.QtCore import QObject, QThread, Signal, Qt
import matplotlib.tri as tri

from widgets import (
    PlotPane, Problems, SummaryPane, confirm_problems, controls_column, draw_mesh, draw_quality_histograms,
    int_spin, placeholder, section, spin,
)


class MeshWorker(QObject):
    finished = Signal(object)
    failed   = Signal(str)

    def __init__(self, cfg):
        super().__init__()
        self.cfg = cfg

    def run(self):
        try:
            self.finished.emit(pycfd.build_heat_mesh(self.cfg))
        except Exception as e:
            self.failed.emit(str(e))


class HeatWorker(QObject):
    finished = Signal(object)
    failed   = Signal(str)

    def __init__(self, cfg, mesh=None):
        super().__init__()
        self.cfg = cfg
        self.mesh = mesh

    def run(self):
        try:
            self.finished.emit(pycfd.run_heat(self.cfg, mesh=self.mesh))
        except Exception as e:
            self.failed.emit(str(e))


def exact_temperature(nodes, cfg):
    return cfg.t_inlet * (1.0 - nodes[:, 0] / cfg.width)


def _nodal_field(axes, result, values, cmap, title, cbar_label):
    triangulated_mesh = tri.Triangulation(result.nodes[:, 0], result.nodes[:, 1], result.elements)
    mappable = axes.tripcolor(triangulated_mesh, values, shading="gouraud", cmap=cmap)
    axes.figure.colorbar(mappable, ax=axes, label=cbar_label)
    axes.set_title(title)
    axes.set_xlabel("X")
    axes.set_ylabel("Y")
    axes.set_aspect("equal")


def render_temperature(axes, result, cfg):
    temperature = np.asarray(result.field)
    _nodal_field(axes, result, temperature, "inferno",
                 f"Temperature — {temperature.min():.2f} to {temperature.max():.2f}", "T")


def render_error(axes, result, cfg):
    if cfg is None:
        placeholder(axes, "no run configuration available")
        return
    error = np.asarray(result.field) - exact_temperature(result.nodes, cfg)
    limit = max(abs(error).max(), 1e-12)
    triangulated_mesh = tri.Triangulation(result.nodes[:, 0], result.nodes[:, 1], result.elements)
    mappable = axes.tripcolor(triangulated_mesh, error, shading="gouraud", cmap="coolwarm",
                              vmin=-limit, vmax=limit)
    axes.figure.colorbar(mappable, ax=axes, label="T - exact")
    axes.set_title(f"Error vs linear ramp — max |err| {abs(error).max():.3g} "
                   f"({100 * abs(error).max() / max(cfg.t_inlet, 1e-12):.3f}% of T_inlet)")
    axes.set_xlabel("X")
    axes.set_ylabel("Y")
    axes.set_aspect("equal")


def render_profile(axes, result, cfg):
    temperature = np.asarray(result.field)
    x = result.nodes[:, 0]
    axes.plot(x, temperature, ".", markersize=4, color="tab:orange", label="FEM nodes")
    if cfg is not None:
        line = np.linspace(0.0, cfg.width, 200)
        axes.plot(line, cfg.t_inlet * (1.0 - line / cfg.width), "-", lw=1.5, color="tab:blue",
                  label="exact  T = T_inlet (1 - x/W)")
    axes.set_title("Temperature along the channel")
    axes.set_xlabel("X")
    axes.set_ylabel("T")
    axes.grid(True, ls=":")
    axes.legend(loc="best")


def heat_summary(result, cfg):
    temperature = np.asarray(result.field)
    rows = [
        ("Nodes", f"{len(result.nodes)}"),
        ("Cells", f"{len(result.elements)}"),
        ("T min", f"{temperature.min():.6f}"),
        ("T max", f"{temperature.max():.6f}"),
        ("T mean", f"{temperature.mean():.6f}"),
    ]
    if cfg is not None:
        error = temperature - exact_temperature(result.nodes, cfg)
        rows += [
            ("", ""),
            ("Exact solution", "T = T_inlet (1 - x/W)"),
            ("Max |error|", f"{abs(error).max():.6g}"),
            ("RMS error", f"{np.sqrt((error ** 2).mean()):.6g}"),
            ("Max error / T_inlet", f"{abs(error).max() / max(cfg.t_inlet, 1e-12):.3%}"),
            ("", ""),
            ("Domain", f"{cfg.width:g} x {cfg.height:g}"),
            ("T_inlet", f"{cfg.t_inlet:g}"),
            ("Mesh density", f"{cfg.density:g}"),
        ]
    return rows


HEAT_PLOTS = [
    ("Temperature", render_temperature),
    ("Profile", render_profile),
    ("Error", render_error),
]


class HeatInputPanel(QWidget):
    def __init__(self):
        super().__init__()
        self.setSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred)
        form = QFormLayout()
        form.setLabelAlignment(Qt.AlignmentFlag.AlignRight)
        form.setFieldGrowthPolicy(QFormLayout.FieldGrowthPolicy.AllNonFixedFieldsGrow)

        section(form, "Domain")
        self.width_field = spin(form, "Width:", value=4.0, low=0.01, high=1000.0, step=0.5,
                                decimals=3, tip="Length of the plate along x, inlet to outlet.")
        self.height_field = spin(form, "Height:", value=2.0, low=0.01, high=1000.0, step=0.5,
                                 decimals=3, tip="Height along y. The top and bottom edges are "
                                                 "insulated, so this does not change the answer.")
        self.density = int_spin(form, "Mesh density:", value=400, low=5, high=5000, step=50,
                                tip="Interior node budget. The heat solve is cheap; 400 gives "
                                    "roughly 900 nodes in well under a second.")

        section(form, "Boundary conditions")
        self.t_inlet = spin(form, "Left edge T:", value=100.0, low=-10_000.0, high=10_000.0,
                            step=10.0, decimals=3,
                            tip="Dirichlet temperature on the inlet (left) edge.")
        outlet = QLabel("Right edge is held at 0. Top and bottom are insulated (zero flux).")
        outlet.setWordWrap(True)
        outlet.setStyleSheet("font-size: 11px; color: gray;")
        form.addRow(outlet)

        exact = QLabel("Exact solution:  T(x) = T_left · (1 − x / width)")
        exact.setStyleSheet("font-size: 11px; color: gray; font-style: italic;")
        form.addRow(exact)

        self.setLayout(form)

    def config(self, problems):
        cfg = pycfd.HeatConfig()
        cfg.width = self.width_field.value()
        cfg.height = self.height_field.value()
        cfg.t_inlet = self.t_inlet.value()
        cfg.density = float(self.density.value())
        self.check(cfg, problems)
        return cfg

    def check(self, cfg, problems):
        if cfg.t_inlet == 0.0:
            problems.error("Left edge T is 0, the same as the right edge — the whole field would "
                           "be zero and the error plots would divide by zero.")
        aspect = max(cfg.width, cfg.height) / min(cfg.width, cfg.height)
        if aspect > 20.0:
            problems.warn(f"The domain is {aspect:.0f}:1. The triangulation will be full of "
                          f"slivers; check the Mesh quality tab afterwards.")
        if cfg.density < 20:
            problems.warn(f"Mesh density {cfg.density:g} gives very few interior nodes, so the "
                          f"boundary groups may not resolve properly.")
        if cfg.density > 2000:
            problems.warn(f"Mesh density {cfg.density:g} is far more than this problem needs — "
                          f"the exact answer is linear, so extra nodes buy nothing.")


class HeatPlotPanel(QTabWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setTabPosition(QTabWidget.TabPosition.South)
        self.setSizePolicy(QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Expanding)

        self.result = None
        self.cfg = None

        self.mesh_pane = PlotPane()
        self.quality_pane = PlotPane()
        self.addTab(self.mesh_pane, "Mesh")
        self.addTab(self.quality_pane, "Mesh quality")
        self.mesh_pane.show_placeholder("run the solver")
        self.quality_pane.show_placeholder("run the solver")

        self.result_panes = []
        for label, renderer in HEAT_PLOTS:
            pane = PlotPane(renderer)
            pane.show_placeholder("run the solver")
            self.addTab(pane, label)
            self.result_panes.append(pane)

        self.summary_pane = SummaryPane(heat_summary)
        self.addTab(self.summary_pane, "Summary")
        self.result_panes.append(self.summary_pane)

        self.currentChanged.connect(self._render_current)

    def set_config(self, cfg):
        self.cfg = cfg

    def _render_current(self, _index=None):
        pane = self.currentWidget()
        if getattr(pane, "dirty", False) and self.result is not None:
            pane.render(self.result, self.cfg)

    def show_busy(self, message):
        pane = self.result_panes[0]
        pane.show_placeholder(message)
        pane.dirty = True
        self.setCurrentWidget(pane)

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
        self.plot_mesh(result.nodes, result.elements, switch=False)
        for pane in self.result_panes:
            pane.dirty = True
        self.setCurrentWidget(self.result_panes[0])
        self._render_current()


class HeatTab(QWidget):
    def __init__(self):
        super().__init__()
        self.input_panel = HeatInputPanel()
        self.plot_panel = HeatPlotPanel()
        self.mesh = None
        self.result = None
        self.thread = None
        self.worker = None
        self._cfg = None
        self._running = False

        self.mesh_btn = QPushButton("Create and Plot Mesh")
        self.run_btn = QPushButton("Run Heat Solve")
        self.status = QLabel("Ready.")
        self.status.setWordWrap(True)
        self.status.setStyleSheet("color: gray; font-size: 12px;")

        controls = QVBoxLayout()
        controls.addWidget(self.input_panel)
        controls.addWidget(self.mesh_btn)
        controls.addWidget(self.run_btn)
        controls.addWidget(self.status)
        controls.addStretch()

        controls_widget = controls_column(controls)

        splitter = QSplitter(Qt.Orientation.Horizontal)
        splitter.addWidget(controls_widget)
        splitter.addWidget(self.plot_panel)
        splitter.setStretchFactor(0, 0)
        splitter.setStretchFactor(1, 1)
        splitter.setSizes([400, 1100])

        layout = QHBoxLayout(self)
        layout.addWidget(splitter)

        self.mesh_btn.clicked.connect(self.start_mesh)
        self.run_btn.clicked.connect(self.start_solve)

    def _config_or_warn(self):
        if self._running:
            return None
        problems = Problems()
        cfg = self.input_panel.config(problems)
        return cfg if confirm_problems(self, "Heat solver", problems) else None

    def start_mesh(self):
        cfg = self._config_or_warn()
        if cfg is None:
            return
        self._cfg = cfg
        self.plot_panel.set_config(cfg)
        self._set_busy(True, "Building mesh…")
        self._start(MeshWorker(cfg), self.on_mesh_ready)

    def start_solve(self):
        cfg = self._config_or_warn()
        if cfg is None:
            return

        self._cfg = cfg
        self.plot_panel.set_config(cfg)
        mesh, self.mesh = self.mesh, None      # run_heat consumes it; None means it meshes its own
        self.plot_panel.show_busy("Solving…")
        self._set_busy(True, "Solving…" if mesh is not None else "Meshing, then solving…")
        self._start(HeatWorker(cfg, mesh), self.on_result)

    def _start(self, worker, on_finished):
        self.thread = QThread()
        self.worker = worker
        worker.moveToThread(self.thread)
        self.thread.started.connect(worker.run)
        worker.finished.connect(on_finished)
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
        self._set_busy(False, f"Mesh ready — {len(mesh.elements)} cells, {len(mesh.nodes)} nodes. "
                              f"Run Heat Solve will reuse it.")

    def on_result(self, result):
        self.result = result
        self.plot_panel.plot_solution(result)
        temperature = np.asarray(result.field)
        error = abs(temperature - exact_temperature(result.nodes, self._cfg)).max()
        self._set_busy(False, f"Solved — {len(result.nodes)} nodes, {len(result.elements)} cells, "
                              f"max error vs exact {error:.4g} "
                              f"({error / max(self._cfg.t_inlet, 1e-12):.3%} of T_inlet)")

    def on_failed(self, message):
        self._set_busy(False, "Failed.")
        self._report(message)

    def _set_busy(self, running, message):
        self._running = running
        self.mesh_btn.setEnabled(not running)
        self.run_btn.setEnabled(not running)
        self.status.setText(message)

    def _report(self, message):
        self.status.setText(message)
        QMessageBox.warning(self, "Heat solver — problem", message)


def build_tabs():
    return [(HeatTab(), "Steady Heat")]
