@echo off
setlocal
cd /d "%~dp0\.."

echo ============================================================
echo Installing OBSMV into OBS Studio
echo ============================================================

set "OBS_DIR=C:\Program Files\obs-studio"
if not exist "%OBS_DIR%" (
    echo [ERROR] OBS Studio not found in %OBS_DIR%
    pause
    exit /b 1
)

net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] Administrator rights needed to write into Program Files.
    echo Requesting elevation...
    powershell -Command "Start-Process cmd -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b 0
)

tasklist /FI "IMAGENAME eq obs64.exe" 2>NUL | find /I /N "obs64.exe">NUL
if %ERRORLEVEL%==0 (
    echo.
    echo [ATTENTION] OBS Studio (obs64.exe) est actuellement ouvert !
    echo Fermez OBS Studio pour debloquer le fichier dll, puis appuyez sur une touche.
    pause
)

echo [1/2] Copying obsmv.dll to %OBS_DIR%\obs-plugins\64bit\...
if not exist "%OBS_DIR%\obs-plugins\64bit" mkdir "%OBS_DIR%\obs-plugins\64bit"
copy /Y "bin\obsmv.dll" "%OBS_DIR%\obs-plugins\64bit\obsmv.dll"
if %ERRORLEVEL% NEQ 0 (
    echo [ERREUR] Impossible d'ecrire obsmv.dll. Verifiez que OBS Studio est bien completement ferme.
    pause
    exit /b 1
)

echo [2/2] Copying data (locales, fonts) to %OBS_DIR%\data\obs-plugins\obsmv\...
if not exist "%OBS_DIR%\data\obs-plugins\obsmv" mkdir "%OBS_DIR%\data\obs-plugins\obsmv"
xcopy /E /I /Y "data" "%OBS_DIR%\data\obs-plugins\obsmv\"

echo.
echo ============================================================
echo [SUCCESS] OBSMV installe avec succes !
echo Vous pouvez maintenant relancer OBS Studio.
echo ============================================================
pause
