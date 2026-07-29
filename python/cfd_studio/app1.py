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

import pycfd 
import numpy as np    

from PySide6.QtWidgets import (
    QApplication, QComboBox, QHBoxLayout, QLineEdit, QMainWindow, QPushButton, QWidget, QVBoxLayout, QTabWidget,
    QFormLayout, QLabel,
)
from PySide6.QtCore import QObject, QThread, Signal
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib.tri as tri

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

    #def run(self):

class InputPanel(QWidget):
    """Solver inputs -> a pycfd.FvmConfig."""
    aerofoilNodes_ready = Signal(object)
    mesh_ready = Signal(object, object)
    result_management_ready = Signal(object)
    def __init__(self):
        super().__init__()
        self.Aerofoil_array = 0
        self.currentmesh = None

        form = QFormLayout()

        self.NACA_digits4 = QComboBox()
        self.NACA_digits4.addItems(["2412" , "2415", "2418"])
        form.addRow("NACA digits4:", self.NACA_digits4)

        self.Density = QComboBox()
        self.Density.addItems(["300", "300", "300", "300", "300"])
        form.addRow("Density:", self.Density)

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

        self.setLayout(form)

    def on_configure(self):
        cfg = self.config()
        Aerofoil_array = pycfd.naca_outline(cfg.naca.digits4, cfg.naca.n_points)
        self.aerofoilNodes_ready.emit(Aerofoil_array)

    def mesh(self):
        print("Beginning Mesh:")
        cfg = self.config()
        self.currentmesh = pycfd.build_aerofoil_mesh(cfg)
        self.mesh_ready.emit(self.currentmesh.nodes, self.currentmesh.elements)

    def calculate(self):
            print("Beginning Solver:")
            cfg = self.config()
            result = pycfd.run_fvm(cfg, cb = None, mesh = self.currentmesh)
            if result is not None:
                self.result_management_ready.emit(result)
                print("cl", result.cl, "watertight", result.watertight,
                "last residual", result.residual_history[-1], "iters", len(result.residual_history))
                self.currentmesh = None

    def config(self):
        """Return a pycfd.FvmConfig from the current input values."""
        cfg = pycfd.FvmConfig()
        naca = pycfd.NacaSpec()
        naca.digits4 = int(self.NACA_digits4.currentText())
        naca.n_points = 160
        cfg.naca = naca
        cfg.density = float(self.Density.currentText())
        cfg.mach = float(self.Mach.currentText())
        cfg.alpha_deg = float(self.AoA.currentText())
        cfg.rho_inf = float(self.Rho.currentText())
        cfg.p_inf = float(self.P_inf.currentText())
        cfg.gamma = float(self.gamma.currentText())
        cfg.R = float(self.R.currentText())
        cfg.cfl = float(self.cfl.currentText())
        cfg.tolerance = float(self.tolerance.currentText())
        cfg.max_iters = int(self.max_iterations.currentText())
        cfg.pressure_field_csv = self.pressureFieldCSV.currentText()
        cfg.forces_csv = self.forcesCSV.currentText()
        return cfg


class PlotPanel(FigureCanvas):
    """A Matplotlib canvas that integrates seamlessly into PySide6 layouts."""
    def __init__(self, parent=None):

        self.fig = Figure(figsize=(6, 4), dpi=100)
        super().__init__(self.fig)
        self.setParent(parent)        
        self.axes = self.fig.add_subplot(111)
        
        self.axes.set_title("Solver Convergence")
        self.axes.set_xlabel("Iterations")
        self.axes.set_ylabel("Residuals")
        self.axes.grid(True)

    def plot_aerofoil(self, mesh_data):
        self.axes.clear()
        self.axes.set_title("Aerofoil Mesh")
        self.axes.set_xlabel("X")
        self.axes.set_ylabel("Y")
        self.axes.grid(True)
        self.axes.plot(mesh_data[:, 0], mesh_data[:, 1], color="red", linewidth=1.8, label="aerofoil surface")
        self.axes.legend(loc="upper right")
        self.draw()

    def plot_mesh(self, nodes, elements):
        self.axes.clear()
        triangulated_mesh = tri.Triangulation(nodes[:, 0], nodes[:, 1], elements)
        self.axes.triplot(triangulated_mesh, color="black", linewidth=0.3)
        self.axes.set_title(f"FVM mesh check ({len(triangulated_mesh.triangles)} cells, {len(nodes)} nodes)")
        self.axes.set_aspect("equal")
        self.axes.grid(True)
        self.axes.legend(loc="upper right")
        self.draw()

    def plot_solution(self, result, mesh_data):
        self.axes.clear()
        triangulated_mesh = tri.Triangulation(result.nodes[:, 0], result.nodes[:, 1], result.elements)
        self.axes.tripcolor(triangulated_mesh, facecolors=result.pressure, cmap="coolwarm")
        self.axes.set_title(f"FVM Pressure Field Plot. c_l = ({result.cl}), c_d = ({result.cd})")
        self.axes.set_aspect("equal")
        self.axes.grid(True)
        self.draw()


class SolverTab(QWidget):
    """One solver's view: inputs + viewport + Run/Stop + HUD + convergence."""
    result_ = Signal(object)
    def __init__(self, title, input_panel_class, solver_function):
        super().__init__()
        self.input_panel = input_panel_class()
        self.plot_panel = PlotPanel()
        self.solve_fn = solver_function
        self.result = None                       

        self.configure_btn = QPushButton("Configure")
        self.run_btn = QPushButton("Triangualate and Plot Mesh")
        self.calc = QPushButton("Run Simulation")


        controls = QVBoxLayout()
        controls.addWidget(self.input_panel)
        controls.addWidget(self.configure_btn)
        controls.addWidget(self.run_btn)
        controls.addWidget(self.calc)
        controls.addStretch()                  

        main_layout = QHBoxLayout(self)
        main_layout.addLayout(controls, 1)
        main_layout.addWidget(self.plot_panel, 3)

        self.configure_btn.clicked.connect(self.input_panel.on_configure)
        self.run_btn.clicked.connect(self.input_panel.mesh)
        self.calc.clicked.connect(self.input_panel.calculate)

        self.input_panel.aerofoilNodes_ready.connect(self.plot_panel.plot_aerofoil)
        self.input_panel.mesh_ready.connect(self.plot_panel.plot_mesh)
        self.input_panel.result_management_ready.connect(self.resultsManager)

    def resultsManager(self, result):
        self.result = result
        self.result_.emit(self.result)
        print(self.result.nodes[1])

    
    def send_result(self):
        result = self.resultsManager()


class MainWindow(QMainWindow):
    """The main window: tabs for each solver."""
    def __init__(self):
        super().__init__()
        self.setWindowTitle("CFD Studio")
        self.tabs = QTabWidget()
        self.tabs.addTab(SolverTab("FVM", InputPanel, pycfd.run_fvm), "FVM Solver")
        self.tabs.addTab(SolverTab("FVM_2", InputPanel, pycfd.run_fvm), "FVM_2 Solver")
        self.setCentralWidget(self.tabs)



def main():
    app = QApplication(sys.argv)
    win = MainWindow()
    win.resize(1200, 800)
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
