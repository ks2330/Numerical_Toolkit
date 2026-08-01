# Numerical Toolkit Studio — Desktop App

**Numerical Toolkit Studio** is the interactive front-end for the toolkit: a PySide6 +
matplotlib desktop app that drives the C++ solvers directly through a compiled pybind11
module (`pycfd`) and renders results in-memory — no CSV files, no subprocesses. This guide
gets it building and running from a fresh clone.

## Requirements

| Tool | Version | Why |
|------|---------|-----|
| CPython | 3.14 | Runs the GUI; the `pycfd` module must be built against this same interpreter. |
| Visual Studio 2022 (MSVC) | — | Builds `pycfd` — a Python extension must match CPython's ABI. |
| CMake | ≥ 3.15 | Configures the C++ build. |

## 1 · Environment

From the repository root:

```powershell
py -3.14 -m venv .venv                 # create the virtual environment
.\.venv\Scripts\Activate.ps1           # PowerShell  (cmd: scripts\activate)
pip install -r requirements.txt        # PySide6, matplotlib, numpy, pybind11
```

Verify:

```powershell
python -c "import PySide6, matplotlib, numpy; print('GUI dependencies OK')"
```

## 2 · Build the `pycfd` module

The GUI imports `pycfd` at start-up, so build it **before** running the app. It builds into
its own `build-py/` tree with MSVC, against the venv's Python and the pip-installed pybind11:

```powershell
cmake -S . -B build-py -G "Visual Studio 17 2022" -A x64 `
  -DBUILD_PYTHON=ON -DBUILD_TESTS=OFF -DBUILD_APPS=OFF -DPYBIND11_FINDPYTHON=ON `
  -DPython_EXECUTABLE=.venv\Scripts\python.exe `
  -Dpybind11_DIR=.venv\Lib\site-packages\pybind11\share\cmake\pybind11

cmake --build build-py --config Release --target pycfd
```

Then make the module importable — copy it next to the app:

```powershell
copy build-py\bindings\Release\pycfd*.pyd python\cfd_studio\
python -c "import pycfd; print('pycfd loaded')"
```

Re-run the `cmake --build` + `copy` after any change to `bindings/bindings.cpp`, and start a
fresh interpreter to pick up the rebuilt `.pyd` (re-`import` in a live REPL won't reload it).

## 3 · Run the app

```powershell
python python\cfd_studio\numerical_toolkit_studio.py
```

## Troubleshooting

- **PowerShell blocks `Activate.ps1`** — run `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`,
  or skip activation and call the venv Python directly:
  `.\.venv\Scripts\python.exe python\cfd_studio\numerical_toolkit_studio.py`.
- **`pip` finds no wheel** — the package hasn't published one for your exact Python; use a
  version with wheels for PySide6 / numpy / matplotlib.
- **`import pycfd` fails** — the `.pyd` must be built against the *same* interpreter you import
  from (step 2 uses the venv Python) and sit beside `numerical_toolkit_studio.py` (or be on
  `PYTHONPATH`).

## Layout (Python side)

```
python/cfd_studio/numerical_toolkit_studio.py   PySide6 app entry point (assembles the tabs)
python/cfd_studio/aerofoil.py                    FVM Euler / potential / Cp-compare / benchmark tabs
python/cfd_studio/heat.py                        steady-heat tab
python/cfd_studio/widgets.py                     shared input fields and plot panes
bindings/bindings.cpp                            the pycfd module — wraps app_support::solver_api
requirements.txt                                 Python dependency pins (repo root)
scripts/activate.bat                             cmd helper to activate the venv
build-py/                                        MSVC build tree for pycfd (git-ignored)
```
