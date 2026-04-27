from PySide6.QtWidgets import (
    QApplication,
    QPushButton,
    QMainWindow,
    QWidget,
    QVBoxLayout,
    QHBoxLayout,
    QLabel,
    QSpinBox,
    QDoubleSpinBox,
    QComboBox,
    QMessageBox,
    QTabWidget,
    QScrollArea,
)
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib.pyplot as plt
import matplotlib.tri as tri
import csv
import sys
from PySide6.QtCore import Qt
import os
import subprocess
import re
from pathlib import Path
import numpy as np


current_dir = Path(__file__).parent.resolve()
project_root = current_dir.parent.parent
script_path = project_root / "build_FEM_Solver.sh"
main_cpp_path = current_dir / "main.cpp"

bash_path = r"C:\Program Files\Git\bin\bash.exe"


def update_main_cpp(nx, ny, segsPerUnit, numRandomNodes, shape):
    """Update main.cpp with new parameter values."""
    try:
        with open(main_cpp_path, "r") as f:
            content = f.read()

        # Replace each parameter
        content = re.sub(r"int nx = \d+;", f"int nx = {nx};", content)
        content = re.sub(r"int ny = \d+;", f"int ny = {ny};", content)
        content = re.sub(
            r"int segsPerUnit = \d+;", f"int segsPerUnit = {segsPerUnit};", content
        )
        content = re.sub(
            r"int numRandomNodes = \d+;",
            f"int numRandomNodes = {numRandomNodes};",
            content,
        )
        content = re.sub(
            r'std::string shape = "[^"]*";', f'std::string shape = "{shape}";', content
        )

        with open(main_cpp_path, "w") as f:
            f.write(content)

        return True
    except Exception as e:
        print(f"Error updating main.cpp: {e}")
        return False


def on_run_clicked():
    """Build and run with current parameters."""
    try:
        subprocess.run([bash_path, str(script_path)], check=True)
        QMessageBox.information(None, "Success", "Build and execution completed!")
    except subprocess.CalledProcessError as e:
        QMessageBox.critical(None, "Error", f"Execution failed: {e}")


def on_apply_clicked(nx_box, ny_box, segs_box, nodes_box, shape_combo):
    """Apply parameters and update main.cpp."""
    nx = nx_box.value()
    ny = ny_box.value()
    segsPerUnit = segs_box.value()
    numRandomNodes = nodes_box.value()
    shape = shape_combo.currentText()

    if update_main_cpp(nx, ny, segsPerUnit, numRandomNodes, shape):
        QMessageBox.information(
            None,
            "Applied",
            f"Parameters updated:\n"
            f"  nx={nx}, ny={ny}\n"
            f"  segsPerUnit={segsPerUnit}\n"
            f"  numRandomNodes={numRandomNodes}\n"
            f"  shape={shape}",
        )
    else:
        QMessageBox.critical(None, "Error", "Failed to update main.cpp")


def read_xy(path: str):
    """Read x, y coordinates from CSV."""
    xs, ys = [], []
    if not os.path.exists(path):
        return np.array([]), np.array([])
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
    return np.array(xs), np.array(ys)


def read_nodes(path: str):
    """Read nodes with temperature from CSV."""
    xs, ys, Ts = [], [], []
    if not os.path.exists(path):
        return np.array([]), np.array([]), np.array([])
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            xs.append(float(row["x"]))
            ys.append(float(row["y"]))
            Ts.append(float(row["temperature"]))
    return np.array(xs), np.array(ys), np.array(Ts)


def read_elements(path: str):
    """Read triangle elements from CSV."""
    triangles = []
    if not os.path.exists(path):
        return np.array([], dtype=int)
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            triangles.append([int(row["n0"]), int(row["n1"]), int(row["n2"])])
    return np.array(triangles, dtype=int)


def draw_triangulation(ax, path):
    """Draw triangulation edges on axis."""
    if not os.path.exists(path):
        return
    first = True
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            xs = [
                float(row["ax"]),
                float(row["bx"]),
                float(row["cx"]),
                float(row["ax"]),
            ]
            ys = [
                float(row["ay"]),
                float(row["by"]),
                float(row["cy"]),
                float(row["ay"]),
            ]
            ax.plot(
                xs,
                ys,
                color="purple",
                linewidth=1.5,
                alpha=0.7,
                label="Triangulation" if first else None,
            )
            first = False


def create_mesh_figure():
    """Create mesh visualization figure."""
    fig = Figure(figsize=(10, 6), dpi=100)
    ax = fig.add_subplot(111)

    draw_triangulation(ax, "triangulation.csv")

    x, y = read_xy("boundary_nodes_rectangular.csv")
    if len(x) > 0:
        ax.scatter(
            x,
            y,
            c="steelblue",
            s=40,
            marker="o",
            edgecolors="black",
            linewidths=0.4,
            zorder=3,
            label="Mesh nodes",
        )

    ax.set_title("Mesh Visualization — Delaunay Triangulation", fontsize=13)
    ax.set_xlabel("x", fontsize=11)
    ax.set_ylabel("y", fontsize=11)
    ax.set_aspect("equal")
    if ax.get_legend_handles_labels()[0]:
        ax.legend(fontsize=10)
    ax.grid(True, linestyle="--", alpha=0.4)
    fig.tight_layout()
    return fig


def create_fem_figure():
    """Create FEM steady-state solution figure."""
    fig = Figure(figsize=(10, 6), dpi=100)
    ax = fig.add_subplot(111)

    nodes_csv = "steady_state_nodes.csv"
    elems_csv = "steady_state_elements.csv"

    x, y, T = read_nodes(nodes_csv)
    triangles = read_elements(elems_csv)

    if len(x) == 0 or len(triangles) == 0:
        ax.text(
            0.5,
            0.5,
            "No FEM data available.\nRun 'Build & Run' first.",
            ha="center",
            va="center",
            transform=ax.transAxes,
            fontsize=12,
        )
        return fig

    triang = tri.Triangulation(x, y, triangles)

    # Smooth temperature field
    tc = ax.tripcolor(triang, T, shading="gouraud", cmap="coolwarm", vmin=0, vmax=100)

    # Overlay mesh
    ax.triplot(triang, color="black", linewidth=0.6, alpha=0.35)

    # Node scatter
    sc = ax.scatter(
        x,
        y,
        c=T,
        cmap="coolwarm",
        vmin=0,
        vmax=100,
        s=40,
        edgecolors="black",
        linewidths=0.5,
        zorder=3,
    )

    # Contour lines
    levels = np.linspace(0, 100, 7)
    ax.tricontour(triang, T, levels=levels, colors="white", linewidths=0.8, alpha=0.7)

    # Colorbar
    cbar = fig.colorbar(tc, ax=ax, pad=0.02)
    cbar.set_label("Temperature (°C)", fontsize=12)
    cbar.set_ticks(levels)

    ax.set_title("Steady-State Heat Distribution — FEM Solution", fontsize=13)
    ax.set_xlabel("x  (m)", fontsize=11)
    ax.set_ylabel("y  (m)", fontsize=11)
    ax.set_aspect("equal")
    fig.tight_layout()
    return fig


def on_plot_mesh_clicked(mesh_canvas):
    """Plot mesh visualization."""
    try:
        fig = create_mesh_figure()
        mesh_canvas.figure = fig
        mesh_canvas.draw()
    except Exception as e:
        QMessageBox.critical(None, "Error", f"Failed to plot mesh: {e}")


def on_plot_fem_clicked(fem_canvas):
    """Plot FEM solution."""
    try:
        fig = create_fem_figure()
        fem_canvas.figure = fig
        fem_canvas.draw()
    except Exception as e:
        QMessageBox.critical(None, "Error", f"Failed to plot FEM: {e}")


# Create application and main window
app = QApplication([])
window = QMainWindow()
window.setWindowTitle("FEM GUI Configuration")
window.setGeometry(100, 100, 1400, 800)

# Main container with tabs
tab_widget = QTabWidget()
window.setCentralWidget(tab_widget)

# ========== TAB 1: CONTROL PANEL ==========
control_tab = QWidget()
control_layout = QVBoxLayout(control_tab)

# Create parameter input fields
# Unit lengths (nx, ny)
nx_layout = QHBoxLayout()
nx_layout.addWidget(QLabel("Domain X (nx):"))
nx_box = QSpinBox()
nx_box.setValue(6)
nx_box.setMinimum(1)
nx_box.setMaximum(100)
nx_layout.addWidget(nx_box)
control_layout.addLayout(nx_layout)

ny_layout = QHBoxLayout()
ny_layout.addWidget(QLabel("Domain Y (ny):"))
ny_box = QSpinBox()
ny_box.setValue(2)
ny_box.setMinimum(1)
ny_box.setMaximum(100)
ny_layout.addWidget(ny_box)
control_layout.addLayout(ny_layout)

# Segments per unit
segs_layout = QHBoxLayout()
segs_layout.addWidget(QLabel("Segments per Unit:"))
segs_box = QSpinBox()
segs_box.setValue(1)
segs_box.setMinimum(1)
segs_box.setMaximum(10)
segs_layout.addWidget(segs_box)
control_layout.addLayout(segs_layout)

# Random nodes
nodes_layout = QHBoxLayout()
nodes_layout.addWidget(QLabel("Random Nodes (scatter):"))
nodes_box = QSpinBox()
nodes_box.setValue(30)
nodes_box.setMinimum(0)
nodes_box.setMaximum(1000)
nodes_layout.addWidget(nodes_box)
control_layout.addLayout(nodes_layout)

# Shape selection
shape_layout = QHBoxLayout()
shape_layout.addWidget(QLabel("Shape:"))
shape_combo = QComboBox()
shape_combo.addItems(["rectangle", "circle", "both"])
shape_layout.addWidget(shape_combo)
control_layout.addLayout(shape_layout)

# Control buttons
button_layout = QHBoxLayout()
apply_btn = QPushButton("Apply Parameters")
apply_btn.clicked.connect(
    lambda: on_apply_clicked(nx_box, ny_box, segs_box, nodes_box, shape_combo)
)
button_layout.addWidget(apply_btn)

run_btn = QPushButton("Build & Run")
run_btn.clicked.connect(on_run_clicked)
button_layout.addWidget(run_btn)

control_layout.addLayout(button_layout)
control_layout.addStretch()

tab_widget.addTab(control_tab, "Configuration")

# ========== TAB 2: MESH VISUALIZATION ==========
mesh_tab = QWidget()
mesh_layout = QVBoxLayout(mesh_tab)

mesh_canvas = FigureCanvas(Figure(figsize=(10, 6), dpi=100))
mesh_layout.addWidget(mesh_canvas)

mesh_btn_layout = QHBoxLayout()
plot_mesh_btn = QPushButton("Plot Mesh")
plot_mesh_btn.clicked.connect(lambda: on_plot_mesh_clicked(mesh_canvas))
mesh_btn_layout.addWidget(plot_mesh_btn)
mesh_btn_layout.addStretch()
mesh_layout.addLayout(mesh_btn_layout)

tab_widget.addTab(mesh_tab, "Mesh Visualization")

# ========== TAB 3: FEM SOLUTION ==========
fem_tab = QWidget()
fem_layout = QVBoxLayout(fem_tab)

fem_canvas = FigureCanvas(Figure(figsize=(10, 6), dpi=100))
fem_layout.addWidget(fem_canvas)

fem_btn_layout = QHBoxLayout()
plot_fem_btn = QPushButton("Plot FEM Solution")
plot_fem_btn.clicked.connect(lambda: on_plot_fem_clicked(fem_canvas))
fem_btn_layout.addWidget(plot_fem_btn)
fem_btn_layout.addStretch()
fem_layout.addLayout(fem_btn_layout)

tab_widget.addTab(fem_tab, "FEM Solution")

window.show()
app.exec()
