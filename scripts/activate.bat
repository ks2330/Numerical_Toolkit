@echo off
REM Activate the project's Python venv in the current cmd session.
REM   Run from a cmd prompt in the repo root:  scripts\activate
REM %~dp0 = this script's folder (scripts\), so ..\.venv points at the repo-root venv.
call "%~dp0..\.venv\Scripts\activate.bat"
