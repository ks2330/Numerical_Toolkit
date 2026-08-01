"""Input fields, validation and plot panes shared by the Numerical Toolkit Studio tabs."""
import numpy as np

from PySide6.QtWidgets import (
    QCheckBox, QComboBox, QDoubleSpinBox, QFileDialog, QFrame, QHBoxLayout, QLabel,
    QLineEdit, QMessageBox, QPushButton, QScrollArea, QSlider, QSpinBox, QVBoxLayout, QWidget
)
from PySide6.QtCore import Qt, Signal
from matplotlib.backends.backend_qtagg import (
    FigureCanvasQTAgg as FigureCanvas,
    NavigationToolbar2QT as NavigationToolbar,
)
from matplotlib.figure import Figure
import matplotlib.tri as tri


class Problems:
    """Collects everything wrong with a configuration so the user sees it all at once,
    split into blockers and things that are legal but probably a mistake."""

    def __init__(self):
        self.errors = []
        self.warnings = []

    def error(self, message):
        self.errors.append(message)

    def warn(self, message):
        self.warnings.append(message)

    def ok(self):
        return not self.errors

    def error_text(self):
        return "\n".join(f"• {message}" for message in self.errors)

    def warning_text(self):
        return "\n".join(f"• {message}" for message in self.warnings)


# ── input fields ─────────────────────────────────────────────────────────────

def spin(form, label, *, value, low, high, step=1.0, decimals=3, suffix="", tip=""):
    box = QDoubleSpinBox()
    box.setRange(low, high)
    box.setDecimals(decimals)
    box.setSingleStep(step)
    box.setValue(value)
    box.setSuffix(suffix)
    box.setKeyboardTracking(False)          # don't act on half-typed numbers
    box.setToolTip(tip or f"{low:g} to {high:g}")
    form.addRow(label, box)
    return box


def int_spin(form, label, *, value, low, high, step=1, suffix="", tip=""):
    box = QSpinBox()
    box.setRange(low, high)
    box.setSingleStep(step)
    box.setValue(value)
    box.setSuffix(suffix)
    box.setGroupSeparatorShown(True)
    box.setKeyboardTracking(False)
    box.setToolTip(tip or f"{low:,} to {high:,}")
    form.addRow(label, box)
    return box


def choice(form, label, options, *, value=None, disabled=(), tip=""):
    """options: [(text, data), ...]. Not editable — a genuine enumeration.
    Pass `disabled` data values to show an option greyed-out and unselectable
    (e.g. a method that is on the roadmap but not yet wired up)."""
    box = QComboBox()
    for text, item in options:
        box.addItem(text, item)
    for item in disabled:
        index = box.findData(item)
        if index >= 0:
            box.model().item(index).setEnabled(False)
    if value is not None:
        index = box.findData(value)
        if index >= 0:
            box.setCurrentIndex(index)
    box.setToolTip(tip)
    form.addRow(label, box)
    return box


def check(form, label, *, checked=True, tip=""):
    box = QCheckBox()
    box.setChecked(checked)
    box.setToolTip(tip)
    form.addRow(label, box)
    return box


class SliderSpin(QWidget):
    """Slider for sweeping plus a spin box for exact entry, kept in sync. Both clamp,
    so an out-of-range value cannot be entered at all."""
    valueChanged = Signal(float)

    def __init__(self, low, high, value, decimals=2, step=None, suffix="", parent=None):
        super().__init__(parent)
        self._scale = 10 ** decimals

        self.spin = QDoubleSpinBox()
        self.spin.setRange(low, high)
        self.spin.setDecimals(decimals)
        self.spin.setSingleStep(step if step is not None else 10 ** -decimals * 10)
        self.spin.setSuffix(suffix)
        self.spin.setKeyboardTracking(False)
        self.spin.setValue(value)

        self.slider = QSlider(Qt.Orientation.Horizontal)
        self.slider.setRange(int(low * self._scale), int(high * self._scale))
        self.slider.setValue(int(value * self._scale))

        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.slider, 1)
        layout.addWidget(self.spin, 0)

        self.slider.valueChanged.connect(self._from_slider)
        self.spin.valueChanged.connect(self._from_spin)

    def _from_slider(self, raw):
        value = raw / self._scale
        if abs(value - self.spin.value()) > 0.5 / self._scale:
            self.spin.setValue(value)

    def _from_spin(self, value):
        raw = int(round(value * self._scale))
        if raw != self.slider.value():
            self.slider.setValue(raw)
        self.valueChanged.emit(value)

    def value(self):
        return self.spin.value()

    def setValue(self, value):
        self.spin.setValue(value)


def slider_spin(form, label, *, value, low, high, decimals=2, step=None, suffix="", tip=""):
    field = SliderSpin(low, high, value, decimals, step, suffix)
    field.spin.setToolTip(tip or f"{low:g} to {high:g}")
    field.slider.setToolTip(field.spin.toolTip())
    form.addRow(label, field)
    return field


class LogSpin(QWidget):
    """Powers of ten only — the natural way to pick a tolerance, and it makes nonsense
    like 0 or 5 impossible to enter."""
    def __init__(self, exponent, low_exp, high_exp, parent=None):
        super().__init__(parent)
        self.spin = QSpinBox()
        self.spin.setRange(low_exp, high_exp)
        self.spin.setValue(exponent)
        self.spin.setPrefix("1e-")
        self.spin.setKeyboardTracking(False)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.spin)

    def value(self):
        return 10.0 ** -self.spin.value()

    def exponent(self):
        return self.spin.value()


def log_spin(form, label, *, exponent, low_exp=1, high_exp=14, tip=""):
    field = LogSpin(exponent, low_exp, high_exp)
    field.spin.setToolTip(tip or f"1e-{low_exp} down to 1e-{high_exp}")
    form.addRow(label, field)
    return field


class PathField(QWidget):
    """Optional output path. Empty means 'do not write', which the C++ honours."""
    def __init__(self, value, caption, parent=None):
        super().__init__(parent)
        self._caption = caption
        self.edit = QLineEdit(value)
        self.edit.setPlaceholderText("empty = don't write this file")
        browse = QPushButton("…")
        browse.setFixedWidth(30)
        browse.clicked.connect(self._browse)
        layout = QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(self.edit, 1)
        layout.addWidget(browse, 0)

    def _browse(self):
        chosen, _ = QFileDialog.getSaveFileName(self, self._caption, self.edit.text(),
                                                "CSV files (*.csv);;All files (*)")
        if chosen:
            self.edit.setText(chosen)

    def value(self):
        return self.edit.text().strip()


def path_field(form, label, *, value, caption="Choose output file", tip=""):
    field = PathField(value, caption)
    field.edit.setToolTip(tip)
    form.addRow(label, field)
    return field


class ListField(QWidget):
    """A comma-separated list of numbers, with a live count and range readout so a typo
    is visible before anything runs."""
    def __init__(self, value, low, high, parent=None):
        super().__init__(parent)
        self.low, self.high = low, high
        self.edit = QLineEdit(value)
        self.readout = QLabel()
        self.readout.setStyleSheet("font-size: 11px; color: gray;")
        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(2)
        layout.addWidget(self.edit)
        layout.addWidget(self.readout)
        self.edit.textChanged.connect(self._refresh)
        self._refresh()

    def _refresh(self):
        problems = Problems()
        values = self.values(problems)
        if problems.errors:
            self.readout.setText(problems.errors[0])
            self.readout.setStyleSheet("font-size: 11px; color: tab:red; color: #c0392b;")
        else:
            self.readout.setText(f"{len(values)} values, {min(values):g} to {max(values):g}")
            self.readout.setStyleSheet("font-size: 11px; color: gray;")

    def values(self, problems):
        found = []
        for part in self.edit.text().replace(";", ",").split(","):
            part = part.strip()
            if not part:
                continue
            try:
                found.append(float(part))
            except ValueError:
                problems.error(f"{part!r} is not a number.")
                return []
        if not found:
            problems.error("give at least one value.")
            return []
        for value in found:
            if not (self.low <= value <= self.high):
                problems.error(f"{value:g} is outside {self.low:g}–{self.high:g}.")
        return found


def list_field(form, label, *, value, low, high, tip=""):
    field = ListField(value, low, high)
    field.edit.setToolTip(tip)
    form.addRow(label, field)
    return field


class NacaField(QWidget):
    """Build the 4-digit code from its three meaningful parts rather than asking the user
    to remember what '2412' encodes. Each part clamps to its legal range."""
    def __init__(self, camber=2, position=4, thickness=12, parent=None):
        super().__init__(parent)
        self.camber = QSpinBox()
        self.camber.setRange(0, 9)
        self.camber.setValue(camber)
        self.camber.setSuffix("%")
        self.camber.setToolTip("Maximum camber, percent of chord. 0 gives a symmetric section.")

        self.position = QSpinBox()
        self.position.setRange(0, 9)
        self.position.setValue(position)
        self.position.setPrefix("0.")
        self.position.setToolTip("Chordwise position of maximum camber, tenths of chord.")

        self.thickness = QSpinBox()
        self.thickness.setRange(1, 40)
        self.thickness.setValue(thickness)
        self.thickness.setSuffix("%")
        self.thickness.setToolTip("Maximum thickness, percent of chord.")

        self.readout = QLabel()
        self.readout.setStyleSheet("font-family: monospace; font-weight: bold;")

        row = QHBoxLayout()
        row.setContentsMargins(0, 0, 0, 0)
        row.setSpacing(3)
        for caption, widget in (("camber", self.camber), ("at", self.position),
                                ("thick", self.thickness)):
            tag = QLabel(caption)
            tag.setStyleSheet("font-size: 11px; color: gray;")
            row.addWidget(tag)
            row.addWidget(widget, 1)

        column = QVBoxLayout(self)
        column.setContentsMargins(0, 0, 0, 0)
        column.setSpacing(1)
        column.addLayout(row)
        column.addWidget(self.readout)

        for box in (self.camber, self.position, self.thickness):
            box.valueChanged.connect(self._refresh)
        self._refresh()

    def _refresh(self):
        self.readout.setText(f"NACA {self.digits4():04d}")

    def digits4(self):
        return self.camber.value() * 1000 + self.position.value() * 100 + self.thickness.value()

    def check(self, problems):
        if self.camber.value() > 0 and self.position.value() == 0:
            problems.error("NACA code: camber is non-zero but its position is 0 — a cambered "
                           "section needs its camber position at 0.1 chord or more.")
        if self.thickness.value() < 6:
            problems.warn(f"NACA code: {self.thickness.value()}% thickness is very thin; the "
                          f"mesh near the trailing edge will be poor.")


def naca_field(form, label, *, camber=2, position=4, thickness=12):
    field = NacaField(camber, position, thickness)
    form.addRow(label, field)
    return field


def controls_column(layout):
    """Wrap the controls in a scroll area: it keeps a tall form reachable on a short
    screen, and its small minimum lets the splitter actually honour the width we ask for
    instead of the form's own minimum taking half the window."""
    inner = QWidget()
    inner.setLayout(layout)
    area = QScrollArea()
    area.setWidgetResizable(True)
    area.setWidget(inner)
    area.setFrameShape(QFrame.Shape.NoFrame)
    area.setHorizontalScrollBarPolicy(Qt.ScrollBarPolicy.ScrollBarAsNeeded)
    area.setMinimumWidth(260)
    return area


def confirm_problems(parent, title, problems):
    """Errors block; warnings ask. Returns True if the run should go ahead."""
    if problems.errors:
        QMessageBox.critical(parent, f"{title} — fix these first", problems.error_text())
        return False
    if problems.warnings:
        answer = QMessageBox.warning(
            parent, f"{title} — are you sure?",
            problems.warning_text() + "\n\nRun anyway?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No)
        return answer == QMessageBox.StandardButton.Yes
    return True


def section(form, title):
    label = QLabel(title)
    label.setStyleSheet("font-weight: bold; color: gray; margin-top: 8px;")
    line = QFrame()
    line.setFrameShape(QFrame.Shape.HLine)
    line.setFrameShadow(QFrame.Shadow.Sunken)
    form.addRow(label)
    form.addRow(line)


# ── plotting ─────────────────────────────────────────────────────────────────

def placeholder(axes, message):
    axes.set_xticks([])
    axes.set_yticks([])
    for spine in axes.spines.values():
        spine.set_visible(False)
    axes.text(0.5, 0.5, message, ha="center", va="center", transform=axes.transAxes, color="grey")


def mesh_quality(nodes, elements):
    corners = nodes[elements]
    a = np.linalg.norm(corners[:, 1] - corners[:, 2], axis=1)
    b = np.linalg.norm(corners[:, 2] - corners[:, 0], axis=1)
    c = np.linalg.norm(corners[:, 0] - corners[:, 1], axis=1)
    with np.errstate(invalid="ignore", divide="ignore"):
        cosines = np.stack([
            (b * b + c * c - a * a) / (2 * b * c),
            (c * c + a * a - b * b) / (2 * c * a),
            (a * a + b * b - c * c) / (2 * a * b),
        ], axis=1)
    angles = np.degrees(np.arccos(np.clip(cosines, -1.0, 1.0)))
    sides = np.stack([a, b, c], axis=1)
    aspect = sides.max(axis=1) / np.maximum(sides.min(axis=1), 1e-30)
    return np.nan_to_num(angles.min(axis=1)), np.nan_to_num(aspect, posinf=0.0)


def draw_quality_histograms(figure, nodes, elements):
    min_angle, aspect = mesh_quality(nodes, elements)
    left, right = figure.subplots(1, 2)

    left.hist(min_angle, bins=60, color="tab:blue")
    left.axvline(30.0, color="tab:red", ls="--", lw=1.0, label="30 deg")
    left.set_title(f"Min angle — worst {min_angle.min():.1f} deg, "
                   f"{(min_angle < 30.0).sum()} cells below 30")
    left.set_xlabel("Smallest angle (deg)")
    left.set_ylabel("Cells")
    left.legend(loc="upper left")
    left.grid(True, ls=":")

    right.hist(np.minimum(aspect, 10.0), bins=60, color="tab:green")
    right.set_title(f"Aspect ratio — worst {aspect.max():.1f}, median {np.median(aspect):.2f}")
    right.set_xlabel("Longest / shortest side (clipped at 10)")
    right.set_ylabel("Cells")
    right.grid(True, ls=":")
    return left


def draw_mesh(axes, nodes, elements, title=None):
    triangulated_mesh = tri.Triangulation(nodes[:, 0], nodes[:, 1], elements)
    axes.triplot(triangulated_mesh, color="black", linewidth=0.3)
    axes.set_title(title or f"Mesh ({len(triangulated_mesh.triangles)} cells, {len(nodes)} nodes)")
    axes.set_xlabel("X")
    axes.set_ylabel("Y")
    axes.set_aspect("equal")
    axes.grid(True)


class PlotPane(QWidget):
    def __init__(self, renderer=None, parent=None):
        super().__init__(parent)
        self.figure = Figure(figsize=(6, 4), dpi=100, layout="constrained")
        self.canvas = FigureCanvas(self.figure)
        self.axes = self.figure.add_subplot(111)
        self._renderer = renderer
        self.dirty = False

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(NavigationToolbar(self.canvas, self))
        layout.addWidget(self.canvas)

    def reset_axes(self):
        self.figure.clear()
        self.axes = self.figure.add_subplot(111)
        return self.axes

    def reset_figure(self):
        self.figure.clear()
        return self.figure

    def show_placeholder(self, message):
        placeholder(self.reset_axes(), message)
        self.canvas.draw_idle()

    def render(self, result, cfg):
        if self._renderer is None:
            return
        self._renderer(self.reset_axes(), result, cfg)
        self.canvas.draw_idle()
        self.dirty = False


class ExplainerPane(QWidget):
    def __init__(self, html, parent=None):
        super().__init__(parent)
        self.dirty = False
        label = QLabel(html)
        label.setWordWrap(True)
        label.setTextFormat(Qt.TextFormat.RichText)
        label.setAlignment(Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignLeft)
        label.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)

        inner = QWidget()
        inner_layout = QVBoxLayout(inner)
        inner_layout.addWidget(label)
        inner_layout.addStretch()

        area = QScrollArea()
        area.setWidgetResizable(True)
        area.setWidget(inner)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(area)

    def render(self, result, cfg):
        self.dirty = False


class SummaryPane(QWidget):
    def __init__(self, summary_fn, parent=None):
        super().__init__(parent)
        self.dirty = False
        self._summary_fn = summary_fn
        self.label = QLabel("Run a simulation to see the summary.")
        self.label.setAlignment(Qt.AlignmentFlag.AlignTop | Qt.AlignmentFlag.AlignLeft)
        self.label.setStyleSheet("font-family: monospace; font-size: 13px;")
        self.label.setTextInteractionFlags(Qt.TextInteractionFlag.TextSelectableByMouse)
        layout = QVBoxLayout(self)
        layout.addWidget(self.label)
        layout.addStretch()

    def render(self, result, cfg):
        rows = self._summary_fn(result, cfg)
        width = max(len(name) for name, _ in rows)
        self.label.setText("\n".join(f"{name.ljust(width)}   {value}".rstrip()
                                     for name, value in rows))
        self.dirty = False
