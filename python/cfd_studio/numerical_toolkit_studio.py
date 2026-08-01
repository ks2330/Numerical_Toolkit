"""
Numerical Toolkit Studio — one PySide6 front-end over every solver in the toolkit.

Talks to the compiled `pycfd` module (pybind11) — no CSV, no subprocess. Run inside
the Python 3.13 venv with PySide6 + matplotlib + numpy installed, and pycfd on the
path (build with -DBUILD_PYTHON=ON, then copy the .pyd next to this file).

    python numerical_toolkit_studio.py

Tabs come from aerofoil.py (FVM Euler, potential flow, Cp comparison, solver
benchmark) and heat.py (steady heat); shared inputs and plot panes are in widgets.py.
"""
import sys

from PySide6.QtWidgets import QApplication, QMainWindow, QTabWidget

import aerofoil
import heat


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Numerical Toolkit Studio")
        self.tabs = QTabWidget()
        self.tabs.setDocumentMode(True)
        self.tabs.setMovable(True)

        self.solver_tabs = {}
        for widget, title in aerofoil.build_tabs() + heat.build_tabs():
            self.tabs.addTab(widget, title)
            self.solver_tabs[title] = widget

        self.setCentralWidget(self.tabs)
        self.statusBar().showMessage(
            "Each tab configures one solver. Hover any input for what it does and its limits.")


def main():
    app = QApplication(sys.argv)
    app.setApplicationName("Numerical Toolkit Studio")
    win = MainWindow()
    win.resize(1500, 900)
    win.show()
    sys.exit(app.exec())


if __name__ == "__main__":
    main()
