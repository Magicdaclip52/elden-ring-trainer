@echo off
setlocal
set "VSROOT=C:\Program Files\Microsoft Visual Studio\18\Community"
set "CMAKE=%VSROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "VCVARS=%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%CMAKE%" (
    echo [ERTrainer] cmake.exe not found: %CMAKE%
    exit /b 1
)
if not exist "%VCVARS%" (
    echo [ERTrainer] vcvars64.bat not found: %VCVARS%
    exit /b 1
)

call "%VCVARS%" >nul
cd /d "%~dp0"

"%CMAKE%" -S . -B build -G "Visual Studio 18 2026" -A x64
if errorlevel 1 exit /b 1

"%CMAKE%" --build build --config Release
if errorlevel 1 exit /b 1

echo.
echo [ERTrainer] OK: build\bin\Release\ERTrainer129.exe
endlocal
