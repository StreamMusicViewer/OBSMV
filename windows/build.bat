@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0\.."

echo ============================================================
echo Building OBSMV - OBS Studio Native Plugin (Release x64)
echo ============================================================

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] Visual Studio Installer vswhere.exe not found.
    exit /b 1
)

for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if "%VS_PATH%"=="" (
    echo [ERROR] Visual Studio C++ x64 Build Tools not found.
    exit /b 1
)

call "%VS_PATH%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 > nul

echo [1/3] Configuring CMake with Ninja...
cmake -B windows\build -G Ninja -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

echo [2/3] Compiling OBSMV Plugin...
cmake --build windows\build -j
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed.
    exit /b 1
)

echo [3/3] Copying output plugin...
if not exist "bin" mkdir "bin"
copy /Y "windows\build\obsmv.dll" "bin\obsmv.dll" > nul

echo.
echo ============================================================
echo [SUCCESS] OBSMV Plugin built successfully!
echo Binary: bin\obsmv.dll
echo ============================================================

