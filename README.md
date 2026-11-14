# OBSMV (OBS Stream Music Visualizer)

![Status](https://img.shields.io/badge/status-active-success)
![Version](https://img.shields.io/badge/version-v1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)
![OBS Studio](https://img.shields.io/badge/OBS%20Studio-30%2B%20%2F%2031%2B%20%2F%2033%2B-black)
![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C)

> **OBSMV** is a native **OBS Studio** plugin (Windows, Linux, macOS) that delivers both a state-of-the-art **audio spectrum visualizer** (in the vein of *Spectralizer*) and an interactive, highly-customizable **Now Playing** music info card rendered **directly by OBS Studio's internal graphics pipeline (GPU)**, with zero browser sources or Chromium Embedded Framework (CEF) overhead.

---

## Highlights & Features

### 1. 100% Native Internal GPU Rendering in OBS Studio
- **Zero Browser Sources**: Hardware-accelerated direct rendering via the libobs graphics pipeline (`gs_*`).
- **Minimal Resource Footprint**: Sub-1% CPU and GPU utilization, with zero Chromium dependencies or RAM overhead.
- **Crisp Vector Typography**: High-fidelity text rasterization powered by `stb_truetype` with 2x supersampling and glyph caching.
- **Integrated System Font Selector**: Pick any font installed on your operating system (Segoe UI, Arial, Roboto, Inter, Bahnschrift, Montserrat, etc.) directly from OBS Studio's native font picker.
- **GPU-Decoded Album Artwork**: Direct hardware texture decoding for PNG and JPEG cover art, featuring customizable corner rounding and optional spinning vinyl animation during playback.

### 2. Built-in Real-Time Audio Visualizer (KissFFT)
- **Real-Time OBS Audio Capture**: Captures audio directly from any OBS audio source (desktop audio, microphone, media source, virtual audio cables, etc.).
- **Embedded FFT Engine (KissFFT)**: Fast Fourier Transform engine embedded directly into the plugin—zero external FFTW3 or runtime DLL dependencies required.
- **4 Visualizer Styles**:
  - **Vertical Bars** (adjustable width, spacing, and height)
  - **Mirrored / Centered Bars** (symmetrical dual-sided spectrum)
  - **Spline Wave / Oscillogram** (Catmull-Rom cubic spline interpolation with 12 subdivision steps for silky-smooth fluid curves, seamlessly grounded to the bottom)
  - **Circular / Radial** (ideal around circular album artwork or spinning vinyl discs)
- **Organic Smoothing & Inertia**: Monstercat-style inertia filter, adjustable gravity decay rate, and sensitivity gain boost.
- **Luminous Crest Line**: Optional high-visibility contour line hugging the top ridge of the wave.

### 3. Dual-Tone Dynamic Colors & Dark / Light Glass Themes
- **Intelligent Dual-Tone Palette Extraction**:
  - **Vibrant Accent Color**: High-saturation midtones extracted for wave crests, visualizer peaks, and progress indicators.
  - **Dominant Primary Color**: 3D histogram tone extraction for background glass tints and visualizer base grounding.
- **Vertical Gradient**: Visualizer blends seamlessly from the vibrant accent color down to the primary cover tone.
- **Photoshop-Style Color Influence**: Adjustable blend slider between custom user-chosen colors and album cover colors (from 0% manual to 100% dynamic).
- **Theme Modes**:
  - **Automatic (`Auto`)**: Automatically analyzes artwork luminance ($Y = 0.299R + 0.587G + 0.114B$) to pick the optimal theme.
  - **Dark Glass**: Deep smoked glass background with crisp white (`#FFFFFF`) and light-grey high-contrast text.
  - **Frosted Light**: Translucent light frosted glass with deep charcoal (`#141418`) and slate high-contrast WCAG typography.
- **Clean Border Geometry**: Rendered without any overlapping rectangle artifacts in the corners.

### 4. Music Information & "Now Playing" Card
- **Intelligent Hybrid Media Mode**:
  - **Native OS Standalone**: Direct integration with system media transport APIs (**Windows SMTC**, **Linux D-Bus MPRIS**, **macOS**).
  - **OSMV Synchronization**: Fully compatible with the [OSMV](https://github.com/StreamMusicViewer/OSMV) companion daemon via IPC pipe (`\\.\pipe\osmv-ipc-v3`) or `current_song.json`.
- **Customizable Card Elements**:
  - Album Artwork (square, rounded, or spinning vinyl disc).
  - Title, Artist, and Album with configurable template strings (`{title}`, `{artist}`, `{album}`).
  - Song progress bar with elapsed and total duration indicators (`01:45 / 03:20`).
  - Frosted glassmorphism background card with editable colors, opacity, and borders.

### 5. Three Registered OBS Sources
1. **`OBSMV (Visualizer + Now Playing)`**: All-in-one unified source combining the audio visualizer and the music info card (configurable arrangement: visualizer on top or bottom, adjustable gap).
2. **`OBSMV Visualizer`**: Standalone audio spectrum visualizer.
3. **`OBSMV Now Playing`**: Standalone music information card.

---

## Project Structure

```
OBSMV/
├── CMakeLists.txt              # Cross-platform CMake configuration
├── data/
│   ├── fonts/
│   │   └── Inter.ttf           # Embedded modern font
│   └── locale/
│       ├── en-US.ini           # English translations
│       └── fr-FR.ini           # French translations
├── deps/
│   └── libobs/                 # OBS Studio headers and import libraries
├── release/                    # Ready-to-publish GitHub release artifacts (v1.0.0)
│   ├── obsmv-v1.0.0-windows-x64.zip
│   ├── obsmv-v1.0.0-windows-x64.zip.sha256
│   ├── install-windows.bat
│   └── RELEASE_NOTES_v1.0.0.md
├── src/
│   ├── obsmv_plugin.cpp        # OBS Studio plugin entry point
│   ├── media/                  # SMTC / MPRIS / OSMV Sync media detection
│   │   ├── media_detector.cpp
│   │   ├── media_linux_mpris.cpp
│   │   ├── media_osmv_sync.cpp
│   │   └── media_win_smtc.cpp
│   ├── render/                 # Native OBS graphics rendering (text, images, card)
│   │   ├── card_renderer.cpp
│   │   ├── color_utils.hpp
│   │   ├── image_renderer.cpp
│   │   └── text_renderer.cpp
│   ├── source/                 # OBS source implementations
│   │   ├── obsmv_source_combined.cpp
│   │   ├── obsmv_source_nowplaying.cpp
│   │   └── obsmv_source_visualizer.cpp
│   ├── third_party/            # KissFFT, stb_image, stb_truetype, json
│   └── visualizer/             # Audio capture & FFT processing
│       ├── audio_capture.cpp
│       ├── fft_processor.cpp
│       └── visualizer_renderer.cpp
├── windows/
│   ├── build.bat               # 1-click Windows build script (Ninja/MSVC)
│   └── install.bat             # 1-click local installer for OBS Studio
├── linux/
│   └── build.sh                # Linux build script
└── .github/workflows/
    └── build.yml               # Multiplatform CI (Win x64, Linux x86_64, macOS Universal)
```

---

## Quick Installation

### Windows (64-bit)
1. Go to the **[Releases](https://github.com/StreamMusicViewer/OBSMV/releases)** page and download `obsmv-v1.0.0-windows-x64.zip`.
2. Extract the archive, close OBS Studio, and run `install.bat`.
3. Re-open OBS Studio and add the **OBSMV** source!

*(Manual installation: copy `obs-plugins\64bit\obsmv.dll` to `C:\Program Files\obs-studio\obs-plugins\64bit\`, and `data\obs-plugins\obsmv\` to `C:\Program Files\obs-studio\data\obs-plugins\obsmv\`)*.

### Linux (Ubuntu / Debian / Arch / Fedora)
```bash
# Dependencies (Ubuntu/Debian)
sudo apt update && sudo apt install cmake ninja-build gcc g++ pkg-config libdbus-1-dev

# Build
./linux/build.sh
```
Copy `bin/obsmv.so` to `~/.config/obs-studio/plugins/obsmv/bin/64bit/` and `data/` to `~/.config/obs-studio/plugins/obsmv/data/`.

### macOS (Apple Silicon arm64 & Intel x86_64)

#### From Pre-built Release
1. Download `obsmv-macos-universal.tar.gz` from the latest **[Releases](https://github.com/StreamMusicViewer/OBSMV/releases)** or build artifacts.
2. Extract the archive and copy the files to your OBS Studio plugin directory:
   ```bash
   mkdir -p "$HOME/Library/Application Support/obs-studio/plugins/obsmv/bin"
   mkdir -p "$HOME/Library/Application Support/obs-studio/plugins/obsmv/data"
   cp package/obs-plugins/obsmv.so "$HOME/Library/Application Support/obs-studio/plugins/obsmv/bin/"
   cp -r package/data/obs-plugins/obsmv/* "$HOME/Library/Application Support/obs-studio/plugins/obsmv/data/"
   ```
3. If macOS blocks the plugin, remove the quarantine flag:
   ```bash
   xattr -cr "$HOME/Library/Application Support/obs-studio/plugins/obsmv"
   ```
4. Restart OBS Studio and add the **OBSMV** sources!

#### Building from Source
```bash
brew install ninja simde
cmake -B macos/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build macos/build -j
```
Copy `macos/build/obsmv.so` to `~/Library/Application Support/obs-studio/plugins/obsmv/bin/` and `data/` to `~/Library/Application Support/obs-studio/plugins/obsmv/data/`.

---

## License

Distributed under the GNU General Public License v3.0 (GPL-3.0) — see [LICENSE](LICENSE) for details.
Developed by [@Ulyxx3](https://github.com/Ulyxx3) for the [StreamMusicViewer](https://github.com/StreamMusicViewer) organization.
