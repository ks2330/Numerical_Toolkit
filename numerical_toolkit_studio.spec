# numerical_toolkit_studio.spec — PyInstaller build for Numerical Toolkit Studio.
#
# Run from the repository root:
#     pyinstaller numerical_toolkit_studio.spec
# Output: dist/NumericalToolkitStudio/  (zip that folder for a GitHub Release).
#
# The pycfd .pyd must already be built and sitting in python/cfd_studio/ before you build
# (see DESKTOP_APP.md, or just run build-release.ps1 which does it for you).

from pathlib import Path

APP_DIR = Path("python/cfd_studio")

# Bundle the compiled solver module (whatever cpXXX-win ABI tag it carries) at the bundle root
# so `import pycfd` resolves in the frozen app.
pycfd_binaries = [(str(p), ".") for p in APP_DIR.glob("pycfd*.pyd")]
if not pycfd_binaries:
    raise SystemExit(
        "pycfd*.pyd not found in python/cfd_studio/ — build it first (see DESKTOP_APP.md)."
    )

a = Analysis(
    [str(APP_DIR / "numerical_toolkit_studio.py")],
    pathex=[str(APP_DIR)],                 # so aerofoil / heat / widgets resolve
    binaries=pycfd_binaries,
    datas=[],                              # PySide6 / matplotlib / numpy hooks pull their own data
    hiddenimports=["matplotlib.backends.backend_qtagg"],
    excludes=["tkinter", "PyQt5", "PyQt6", "PySide2", "pytest", "IPython"],
    noarchive=False,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name="NumericalToolkitStudio",
    console=False,                         # windowed GUI — no terminal window
    disable_windowed_traceback=False,
)

coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=False,
    name="NumericalToolkitStudio",
)
