# Virtual Environment Setup Guide

This guide explains how to set up and activate the Python virtual environment for the Numerical Toolkit project.

## Prerequisites

- Python 3.10+ installed and in PATH
- Git installed
- On Windows: Administrator access may be required for some operations

## Setup Instructions

### 1. Create Virtual Environment

Open PowerShell or Command Prompt in the project root and run:

```powershell
python -m venv venv
```

This creates a `venv/` folder with an isolated Python environment.

### 2. Activate Virtual Environment

#### On Windows (PowerShell):
```powershell
.\venv\Scripts\Activate.ps1
```

#### On Windows (Command Prompt):
```cmd
venv\Scripts\activate.bat
```

#### On Linux/Mac:
```bash
source venv/bin/activate
```

After activation, your terminal prompt should change to show `(venv)`.

### 3. Install Dependencies

```bash
pip install --upgrade pip
pip install -r requirements.txt
```

### 4. Verify Installation

```bash
python -c "import build123d, PySide6, matplotlib, numpy; print('All packages installed successfully!')"
```

## Deactivating Virtual Environment

```bash
deactivate
```

## How to Run Python Scripts

With the virtual environment activated, run any Python script:

```bash
python apps/UI/GUI.py
python apps/UI/plot_steady_state.py
python apps/UI/CAD.py
```

## Git Integration

The `.gitignore` file already includes `venv/`, so your virtual environment won't be committed to Git. When cloning the project on another machine, you only need to create a new `venv/` and run the setup steps above.

## Troubleshooting

### PowerShell Execution Policy Error

If you get an error about execution policies, run:
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### Pip Install Fails

Ensure the virtual environment is activated (you should see `(venv)` in your prompt).

### Module Not Found

1. Verify the virtual environment is activated
2. Run `pip install -r requirements.txt` again
3. Try `pip list` to see installed packages

## Project Structure

- `venv/` — Virtual environment folder (ignored by Git)
- `requirements.txt` — Dependency specifications
- `apps/UI/` — Python GUI and plotting scripts
- `CMakeLists.txt` — C++ build configuration

## For Team Members

New team members should:
1. Clone the repository
2. Follow steps 1-3 above
3. Run the appropriate Python scripts

The virtual environment ensures everyone has identical dependencies, avoiding "works on my machine" issues.
