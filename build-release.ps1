# build-release.ps1 — produce a standalone Windows build of Numerical Toolkit Studio.
#
# Prereqs: the .venv with requirements installed (incl. pyinstaller), and Visual Studio 2022
#          (MSVC) to build pycfd. Run from the repository root:  .\build-release.ps1
# Result:  dist\NumericalToolkitStudio\  and  NumericalToolkitStudio-win64.zip
#          (upload the .zip to a GitHub Release; users unzip and run the .exe — no Python needed).

$ErrorActionPreference = "Stop"
$py = ".\.venv\Scripts\python.exe"

Write-Host "1/4  Building pycfd (MSVC, Release)..." -ForegroundColor Cyan
cmake -S . -B build-py -G "Visual Studio 17 2022" -A x64 `
  -DBUILD_PYTHON=ON -DBUILD_TESTS=OFF -DBUILD_APPS=OFF -DPYBIND11_FINDPYTHON=ON `
  -DPython_EXECUTABLE=.venv\Scripts\python.exe `
  -Dpybind11_DIR=.venv\Lib\site-packages\pybind11\share\cmake\pybind11
cmake --build build-py --config Release --target pycfd

Write-Host "2/4  Copying pycfd next to the app..." -ForegroundColor Cyan
Copy-Item build-py\bindings\Release\pycfd*.pyd python\cfd_studio\ -Force

Write-Host "3/4  Running PyInstaller..." -ForegroundColor Cyan
& $py -m PyInstaller --noconfirm --clean --workpath build-pyinstaller numerical_toolkit_studio.spec

Write-Host "4/4  Zipping the dist folder..." -ForegroundColor Cyan
$zip = "NumericalToolkitStudio-win64.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path dist\NumericalToolkitStudio\* -DestinationPath $zip

Write-Host "Done -> $zip" -ForegroundColor Green
Write-Host "Test dist\NumericalToolkitStudio\NumericalToolkitStudio.exe on a clean machine, then attach the zip to a GitHub Release."
