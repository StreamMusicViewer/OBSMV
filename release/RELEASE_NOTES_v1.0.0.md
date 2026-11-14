# OBSMV v1.0.0 — Initial Release 🎉

We are thrilled to announce the first official release of **OBSMV (OBS Stream Music Visualizer)**! 

OBSMV is a **100% native C++20 plugin for OBS Studio** (Windows, Linux, macOS) designed to deliver both a state-of-the-art **real-time audio spectrum visualizer** (similar to *Spectralizer*) and an interactive, highly-customizable **Now Playing music info card** directly rendered by OBS's internal GPU pipeline (`gs_*`), **without any browser sources or Chromium (CEF) overhead**.

---

## ✨ Key Features & Highlights

### ⚡ 100% Native OBS GPU Rendering
- **Zero Browser Source Overhead**: Built with native libobs graphics APIs (`gs_matrix_*`, `gs_draw_*`, `gs_effect_*`), eliminating the CPU and RAM tax of Chromium Embedded Framework (CEF).
- **Sub-1% Resource Footprint**: Real-time rendering with ultra-low latency, synchronized perfectly to OBS render ticks (60+ FPS).
- **Retina-crisp Typography**: Text rendering powered by `stb_truetype` with 2x supersampling, font glyph caching, and native TrueType/OpenType rasterization.
- **Hardware-decoded Album Artwork**: Cover art is directly uploaded to GPU textures with smooth rounded corners and optional animated vinyl rotation during playback.

### 🎵 Advanced Audio Visualizer (Powered by KissFFT)
- **Built-in FFT Engine**: Integrated embedded KissFFT engine—zero external FFTW3 or dynamic library dependencies to install.
- **4 Visualizer Styles**:
  - **Bars**: Classic vertical frequency bars with configurable width, spacing, and height.
  - **Mirrored**: Centered, symmetrical dual-sided spectrum bars.
  - **Wave**: Continuous frequency wave with **Catmull-Rom cubic spline interpolation** (12 subdivision steps) for silky-smooth fluid curves, seamlessly grounded to the base.
  - **Circular / Radial**: Radial frequency spectrum, perfect around a circular album artwork or spinning vinyl.
- **Organic Inertia & Smoothing**: Monstercat-style inertia smoothing, adjustable gravity decay rate, and sensitivity boost.
- **Luminous Crest Line**: Optional high-visibility contour line hugging the top ridge of the wave.

### 🎨 Dual-Tone Dynamic Palette & Dark/Light Glass Themes
- **Dual Palette Extraction**:
  - **Vibrant Accent Color**: High-saturation midtones extracted for wave crests, visualizer peaks, and progress indicators.
  - **Dominant Primary Color**: 3D histogram tone extraction for background glass tints and visualizer base grounding.
  - **Vertical Gradient**: Visualizer blends seamlessly from the vibrant accent color down to the primary cover tone.
- **Photoshop-Style Color Influence**: Adjustable blend slider between custom user-chosen colors and album cover colors (from 0% manual to 100% dynamic).
- **Dark / Light Mode**:
  - **Auto**: Automatically analyzes artwork luminance ($Y = 0.299R + 0.587G + 0.114B$) to choose optimal theme.
  - **Dark Glass**: Deep smoked glass background with crisp white (`#FFFFFF`) and light-grey text.
  - **Frosted Light**: Translucent light glass background with deep charcoal (`#141418`) and slate text.
- **WCAG High-Contrast Typography**: Text colors automatically adapt for maximum readability across any album artwork.

### 🔤 System Font Selector
- Integrated directly with the native OBS Studio font selector (`obs_properties_add_font`).
- Pick **any font installed on your machine** (Segoe UI, Arial, Roboto, Inter, Bahnschrift, Montserrat, etc.).
- Native GDI extraction on Windows loads font tables directly from memory without requiring manual file paths.
- Standalone custom `.ttf` / `.otf` file paths remain supported for custom fonts.

### 🔄 Seamless Hybrid Music Detection
- **Native OS Integration**: Directly interfaces with **Windows SMTC (WinRT)** and **Linux D-Bus (MPRIS)** for automatic playback detection (Spotify, Apple Music, Tidal, YouTube Music, Chrome, Firefox, etc.).
- **OSMV Daemon Synchronization**: Fully compatible with the [OSMV](https://github.com/StreamMusicViewer/OSMV) companion daemon via IPC pipe (`\\.\pipe\osmv-ipc-v3`) or `current_song.json`.

### 📦 Three Registered OBS Sources
1. **`OBSMV (Visualizer + Now Playing)`**: All-in-one unified source with configurable layout (visualizer on top or bottom, adjustable gap).
2. **`OBSMV Visualizer`**: Standalone audio visualizer listening to any OBS audio source.
3. **`OBSMV Now Playing`**: Standalone music information card.

---

## 📥 Installation

### Windows (64-bit)
1. Download **`obsmv-v1.0.0-windows-x64.zip`** below and extract it.
2. Close **OBS Studio** if it is open.
3. Double-click **`install.bat`** (it will request administrator elevation automatically to copy files to `C:\Program Files\obs-studio\`).
4. Re-open OBS Studio and add the **OBSMV** source!

*(Manual installation: copy `obs-plugins\64bit\obsmv.dll` to `C:\Program Files\obs-studio\obs-plugins\64bit\`, and `data\obs-plugins\obsmv\` to `C:\Program Files\obs-studio\data\obs-plugins\obsmv\`)*.

### Linux (Ubuntu / Debian / Arch / Fedora)
Download the Linux archive or build from source:
```bash
git clone https://github.com/StreamMusicViewer/OBSMV.git
cd OBSMV
./linux/build.sh
```
Copy `bin/obsmv.so` to `~/.config/obs-studio/plugins/obsmv/bin/64bit/` and `data/` to `~/.config/obs-studio/plugins/obsmv/data/`.

### macOS (Apple Silicon & Intel Universal)
Build universal binary from source:
```bash
cmake -B macos/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build macos/build -j
```
Copy `macos/build/obsmv.so` to `~/Library/Application Support/obs-studio/plugins/obsmv/bin/`.

---

## 🔒 Checksums

| File | SHA-256 Checksum |
| :--- | :--- |
| `obsmv-v1.0.0-windows-x64.zip` | `589f817f1db85a9cfd92657b2b3edbad0bd45a3b09c87b0d02278b8a1bc24e35` |

---

*Developed with passion by [@Ulyxx3](https://github.com/Ulyxx3) for the [StreamMusicViewer](https://github.com/StreamMusicViewer) organization.*
