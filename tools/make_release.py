import os
import shutil
import zipfile
import hashlib

def create_release():
    base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    release_dir = os.path.join(base_dir, "release")
    pkg_name = "obsmv-v1.0.0-windows-x64"
    staging_dir = os.path.join(release_dir, pkg_name)

    if os.path.exists(staging_dir):
        shutil.rmtree(staging_dir)
    os.makedirs(staging_dir, exist_ok=True)

    # Destination directories
    obs_plugins_dir = os.path.join(staging_dir, "obs-plugins", "64bit")
    data_fonts_dir = os.path.join(staging_dir, "data", "obs-plugins", "obsmv", "fonts")
    data_locale_dir = os.path.join(staging_dir, "data", "obs-plugins", "obsmv", "locale")

    os.makedirs(obs_plugins_dir, exist_ok=True)
    os.makedirs(data_fonts_dir, exist_ok=True)
    os.makedirs(data_locale_dir, exist_ok=True)

    # 1. Copy DLL
    src_dll = os.path.join(base_dir, "bin", "obsmv.dll")
    if not os.path.exists(src_dll):
        raise FileNotFoundError(f"Missing {src_dll}. Build project first.")
    shutil.copy2(src_dll, os.path.join(obs_plugins_dir, "obsmv.dll"))

    # 2. Copy Fonts
    src_font = os.path.join(base_dir, "data", "fonts", "Inter.ttf")
    if os.path.exists(src_font):
        shutil.copy2(src_font, os.path.join(data_fonts_dir, "Inter.ttf"))

    # 3. Copy Locales
    src_locale = os.path.join(base_dir, "data", "locale")
    for f in os.listdir(src_locale):
        if f.endswith(".ini"):
            shutil.copy2(os.path.join(src_locale, f), os.path.join(data_locale_dir, f))

    # 4. Create Release install.bat inside archive
    install_bat_content = """@echo off
setlocal
cd /d "%~dp0"

echo ============================================================
echo   Installing OBSMV v1.0.0 for OBS Studio (64-bit)
echo ============================================================
echo.

set "OBS_DIR=C:\\Program Files\\obs-studio"
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
    powershell -Command "Start-Process cmd -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
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
if not exist "%OBS_DIR%\\obs-plugins\\64bit" mkdir "%OBS_DIR%\\obs-plugins\\64bit"
copy /Y "obs-plugins\\64bit\\obsmv.dll" "%OBS_DIR%\\obs-plugins\\64bit\\obsmv.dll"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to write obsmv.dll. Make sure OBS is completely closed.
    pause
    exit /b 1
)

echo [2/2] Copying data files (fonts, locales)...
if not exist "%OBS_DIR%\\data\\obs-plugins\\obsmv" mkdir "%OBS_DIR%\\data\\obs-plugins\\obsmv"
xcopy /E /I /Y "data\\obs-plugins\\obsmv" "%OBS_DIR%\\data\\obs-plugins\\obsmv" >nul

echo.
echo ============================================================
echo   [SUCCESS] OBSMV v1.0.0 installed successfully!
echo   You can now launch OBS Studio and add the OBSMV source.
echo ============================================================
echo.
pause
"""
    with open(os.path.join(staging_dir, "install.bat"), "w", encoding="utf-8") as f:
        f.write(install_bat_content)

    # 5. Create Release README.txt inside archive
    readme_txt_content = """============================================================
OBSMV - OBS Stream Music Visualizer (v1.0.0)
Native OBS Studio Plugin (C++20 / libobs GPU rendering)
============================================================

Official Release: https://github.com/StreamMusicViewer/OBSMV

--- QUICK INSTALLATION (WINDOWS) ---

Method 1 (Automatic):
  1. Extract this entire zip archive to a folder.
  2. Make sure OBS Studio is closed.
  3. Double-click "install.bat" (it will automatically ask for admin rights).
  4. Launch OBS Studio!

Method 2 (Manual):
  1. Copy "obs-plugins\\64bit\\obsmv.dll" to:
     C:\\Program Files\\obs-studio\\obs-plugins\\64bit\\
  2. Copy "data\\obs-plugins\\obsmv" to:
     C:\\Program Files\\obs-studio\\data\\obs-plugins\\obsmv\\
  3. Launch OBS Studio!

--- FEATURES ---
- 100% Native GPU rendering (Direct3D 11 / OpenGL via libobs gs_* API)
- Zero Browser Source / zero CEF overhead (ultra-low CPU/RAM usage)
- Real-Time Audio Visualizer (Bars, Mirrored, Spline Wave, Circular)
- Catmull-Rom cubic spline interpolation for ultra-smooth fluid curves
- Dynamic 2-Color Palette: extracts vibrant accent + dominant primary base from album art
- Adaptive Dark / Light frosted glass themes with high-contrast typography
- Native Windows Font Selector: pick any font installed on your PC
- Multi-Source Integration:
    * "OBSMV (Visualizer + Now Playing)" - All-in-one combined source
    * "OBSMV Visualizer" - Dedicated audio visualizer
    * "OBSMV Now Playing" - Dedicated music card
- Seamless hybrid media detection (Windows SMTC native + OSMV daemon sync)

--- LICENSE ---
GNU General Public License v3.0 (GPL-3.0)
Developed by Ulyxx3 for the StreamMusicViewer organization.
"""
    with open(os.path.join(staging_dir, "README.txt"), "w", encoding="utf-8") as f:
        f.write(readme_txt_content)

    # 6. Create ZIP archive
    zip_path = os.path.join(release_dir, f"{pkg_name}.zip")
    with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for root, dirs, files in os.walk(staging_dir):
            for file in files:
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, staging_dir)
                zipf.write(full_path, rel_path)

    # 7. Compute SHA256 checksum
    hasher = hashlib.sha256()
    with open(zip_path, 'rb') as f:
        while chunk := f.read(65536):
            hasher.update(chunk)
    sha256_hash = hasher.hexdigest()

    sha256_file = os.path.join(release_dir, f"{pkg_name}.zip.sha256")
    with open(sha256_file, "w", encoding="utf-8") as f:
        f.write(f"{sha256_hash} *{pkg_name}.zip\n")

    # 8. Create standalone install-windows.bat in release/
    with open(os.path.join(release_dir, "install-windows.bat"), "w", encoding="utf-8") as f:
        f.write(install_bat_content)

    print(f"Created: {zip_path}")
    print(f"Size: {os.path.getsize(zip_path)} bytes")
    print(f"SHA-256: {sha256_hash}")

if __name__ == "__main__":
    create_release()
