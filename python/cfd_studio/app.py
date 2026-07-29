"""
CFD Studio — PySide6 front-end for the Numerical Toolkit solvers.

Talks to the compiled `pycfd` module (pybind11) — no CSV, no subprocess. Run inside
the Python 3.13 venv with PySide6 + matplotlib + numpy installed, and pycfd on the
path (build with -DBUILD_PYTHON=ON, then copy/point PYTHONPATH at the .pyd).

Fill in the TODOs (see the plan, Part C): build the input panel, run solves on a
QThread, and render the mesh field with matplotlib tripcolor.
"""
import sys
from unittest import case

import pycfd            # TODO: the compiled pybind11 module
import numpy as np      # TODO: for array handling

from PySide6.QtWidgets import (
    QApplication, QComboBox, QHBoxLayout, QLineEdit, QMainWindow, QPushButton, QWidget, QVBoxLayout, QTabWidget,
    QFormLayout, QLabel,
)
from PySide6.QtCore import QObject, QThread, Signal
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure


class SolverWorker(QObject):
    """Runs a solve off the UI thread so the window stays responsive."""
    finished = Signal(object)
    progress = Signal(int, float) 
    failed   = Signal(str)

    def __init__(self, config):
        super().__init__()
        self._config = config
        self._cancel = False

    def cancel(self):
        self._cancel = True

    def run(self):
        # TODO: call pycfd.run_fvm(self._config, progress_cb) where progress_cb emits
        #       self.progress and returns `not self._cancel`. Wrap in try/except and
        #       emit self.failed(str(e)) on the RuntimeError the solver throws when it
        #       diverges. On success emit self.finished(result).
        pycfd.run_fvm(self._config, self.progress_callback)



class MeshCanvas(FigureCanvas):
    """Matplotlib canvas that renders the mesh scalar field."""
    def __init__(self):
        self.fig = Figure(figsize=(6, 5))
        super().__init__(self.fig)
        self.ax = self.fig.add_subplot(111)

    def draw_field(self, nodes, elements, cell_values, airfoil=None, label="pressure"):
        # TODO: matplotlib.tri.Triangulation(nodes[:,0], nodes[:,1], elements);
        #       ax.tripcolor(triang, facecolors=cell_values, cmap="coolwarm")  # flat per-cell (FVM)
        #       colorbar; overlay `airfoil` outline; ax.set_aspect("equal"); self.draw()
        raise NotImplementedError("MeshCanvas.draw_field")

class MeshPanel(QWidget):
    """Mesh plot."""
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout(self)
        self.plot = MeshCanvas()
        layout.addWidget(self.plot)

class InputPanel(QWidget):
    """Solver inputs -> a pycfd.FvmConfig."""
    def __init__(self):
        super().__init__()
        form = QFormLayout()

        self.NACA_digits4 = QComboBox()
        self.NACA_digits4.addItems(["2412" , "2415", "2418"])
        form.addRow("NACA digits4:", self.NACA_digits4)

        self.Mach = QComboBox()
        self.Mach.addItems(["0.3", "0.4", "0.5", "0.6", "0.7", "0.8", "0.9", "1.0"])
        form.addRow("Mach:", self.Mach)

        self.AoA = QComboBox()
        self.AoA.addItems(["2", "4", "6", "8", "10", "12", "14", "16", "18", "20"])
        form.addRow("AoA:", self.AoA)  

        self.Rho = QComboBox()
        self.Rho.addItems(["1", "1.1", "1.2", "1.3", "1.4"])
        form.addRow("Free Stream Density:", self.Rho)

        self.P_inf = QComboBox()
        self.P_inf.addItems(["1", "1.1", "1.2", "1.3", "1.4"])
        form.addRow("Free Stream Pressure:", self.P_inf)

        self.gamma = QComboBox()
        self.gamma.addItems(["1.4", "1.5", "1.6", "1.7", "1.8", "1.9", "2.0"])
        form.addRow("Specific Heat Ratio (γ):", self.gamma)

        self.R = QComboBox()
        self.R.addItems(["287", "300", "400", "500", "600", "700", "800", "900", "1000"])
        form.addRow("Gas constant (R):", self.R)

        self.cfl = QComboBox()
        self.cfl.addItems(["0.5", "0.6", "0.7", "0.8", "0.9", "1.0"])
        form.addRow("CFL:", self.cfl)

        self.tolerance = QComboBox()
        self.tolerance.addItems(["1e-6", "1e-7", "1e-8", "1e-9", "1e-10"])
        form.addRow("Tolerance:", self.tolerance)

        self.max_iterations = QComboBox()
        self.max_iterations.addItems(["100000", "200000", "300000", "400000", "500000"])
        form.addRow("Max iterations:", self.max_iterations)

        self.Pi = QComboBox()
        self.Pi.addItems(["3.141592653589793"])
        form.addRow("Pi:", self.Pi)

        self.pressureFieldCSV = QComboBox()
        self.pressureFieldCSV.addItems(["results/csv/fvm_pressure_field.csv"])
        form.addRow("Pressure Field CSV Result Output:", self.pressureFieldCSV)

        self.forcesCSV = QComboBox()
        self.forcesCSV.addItems(["results/csv/fvm_forces.csv"])
        form.addRow("Forces CSV Result Output:", self.forcesCSV)

        self.submit_button = QPushButton()
        self.submit_button.setText("Configure")
        self.submit_button.clicked.connect(self.on_configure)

        form.addRow(self.submit_button)
        self.setLayout(form)

    def on_configure(self):
        cfg = self.config()
        print("Configured with:", cfg)

    def config(self):
        """Return a pycfd.FvmConfig from the current input values."""
        cfg = pycfd.FvmConfig()
        cfg.naca = pycfd.NacaSpec(int(self.NACA_digits4.currentText()), 160)
        cfg.mach = float(self.Mach.currentText())
        cfg.alphaDeg = float(self.AoA.currentText())
        cfg.rhoInf = float(self.Rho.currentText())
        cfg.pInf = float(self.P_inf.currentText())
        cfg.cfl = float(self.cfl.currentText())
        cfg.gamma = float(self.gamma.currentText())
        cfg.R = float(self.R.currentText())
        cfg.tolerance = float(self.tolerance.currentText())
        cfg.max_iterations = int(self.max_iterations.currentText())
        cfg.Pi = float(self.Pi.currentText())
        cfg.pressureFieldCSV = self.pressureFieldCSV.currentText()
        cfg.forcesCSV = self.forcesCSV.currentText()
        return cfg


class SolverTab(QWidget):
    """One solver's view: inputs + viewport + Run/Stop + HUD + convergence."""
    def __init__(self, title, input_panel_class, solver_function):
        super().__init__()
        self.input_panel = input_panel_class()
        self.solve_fn = solver_function
        # TODO: wire a Run button -> SolverWorker on a QThread -> MeshCanvas.draw_field on finished.
        main_layout = QVBoxLayout(self)

        layout_config = QHBoxLayout()
        layout_config.addWidget(self.input_panel)
        
        layout_plot = QVBoxLayout(self)
        layout_plot.addWidget(MeshPanel())


class BenchmarkTab(QWidget):
    """Benchmark tab: read results/csv/bench.csv and plot."""
    def __init__(self):
        super().__init__()
        layout = QVBoxLayout(self)
        layout.addWidget(QLabel("TODO: Benchmark tab — read results/csv/bench.csv and plot"))


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("CFD Studio")
        tabs = QTabWidget()
        # MainWindow — the caller knows the specifics, so it supplies them:
        tabs.addTab(SolverTab("Euler (FVM)", InputPanel, pycfd.run_fvm), "Euler (FVM)")
        tabs.addTab(SolverTab("Potential (FEM)", InputPanel, pycfd.run_fvm), "Potential (FEM)")
        tabs.addTab(SolverTab("Heat (FEM)", InputPanel, pycfd.run_fvm), "Heat (FEM)")
        tabs.addTab(BenchmarkTab(), "Benchmark")
        # TODO (P7): a "Benchmark" tab reading results/csv/bench.csv (or pycfd.run_benchmark).
        self.setCentralWidget(tabs)


def main():
    app = QApplication(sys.argv)
    win = MainWindow()
    win.resize(1200, 800)
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
