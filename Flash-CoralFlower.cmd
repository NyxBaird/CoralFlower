@echo off
cd /d "%~dp0"
where py >nul 2>nul
if errorlevel 1 (python tools\launch_flasher.py) else (py -3 tools\launch_flasher.py)
if errorlevel 1 pause
