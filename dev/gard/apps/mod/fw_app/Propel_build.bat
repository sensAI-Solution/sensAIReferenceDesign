set PROPEL_VER=2025.1

@echo off
set PROPEL_PATH=C:\lscc\propel\%PROPEL_VER%
echo.
echo To build app module, type: make build_app_module [PROJECT=your_project]
if NOT EXIST %PROPEL_PATH%\ (
    echo ERROR: Lattice Propel not found - check that Propel is installed in: %PROPEL_PATH%\
)

REM Add all Propel required bins to PATH
set PATH=%PROPEL_PATH%\sdk\build_tools\bin;%PATH%
set PATH=%PROPEL_PATH%\sdk\riscv-none-embed-gcc\bin;%PATH%
set PATH=%PROPEL_PATH%\sdk\tools\bin;%PATH%

cmd /K "cd ."
