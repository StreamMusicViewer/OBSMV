@echo off
setlocal
cd /d "%~dp0"

echo ============================================================
echo   Installing OBSMV v1.0.0 for OBS Studio (64-bit)
echo ============================================================
echo.

set "OBS_DIR=C:\Program Files\obs-studio"
if not exist "%OBS_DIR%" (
    echo [ERROR] OBS Studio not found in %OBS_DIR%
    echo Please make sure OBS Studio is installed in standard directory.
    pause
    exit /b 1
)

net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] Administrator rights needed to write to %OBS_DIR%.
    echo Requesting elevation...
    powershell -Command "Start-Process cmd -ArgumentList '/c ""%~f0""' -Verb RunAs"
    exit /b 0
)

tasklist /FI "IMAGENAME eq obs64.exe" 2>NUL | find /I /N "obs64.exe">NUL
if %ERRORLEVEL%==0 (
    echo.
    echo [ATTENTION] OBS Studio (obs64.exe) is currently running!
    echo Please close OBS Studio now so the plugin DLL can be updated.
    echo Press any key when OBS Studio is closed...
    pause >nul
)

echo [1/2] Copying obsmv.dll...
if not exist "%OBS_DIR%\obs-plugins\64bit" mkdir "%OBS_DIR%\obs-plugins\64bit"
copy /Y "obs-plugins\64bit\obsmv.dll" "%OBS_DIR%\obs-plugins\64bit\obsmv.dll"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to write obsmv.dll. Make sure OBS is completely closed.
    pause
    exit /b 1
)

echo [2/2] Copying data files (fonts, locales)...
if not exist "%OBS_DIR%\data\obs-plugins\obsmv" mkdir "%OBS_DIR%\data\obs-plugins\obsmv"
xcopy /E /I /Y "data\obs-plugins\obsmv" "%OBS_DIR%\data\obs-plugins\obsmv" >nul

echo.
echo ============================================================
echo   [SUCCESS] OBSMV v1.0.0 installed successfully!
echo   You can now launch OBS Studio and add the OBSMV source.
echo ============================================================
echo.
pause
