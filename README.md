# OBSMV (OBS Stream Music Visualizer)

![Status](https://img.shields.io/badge/status-active-success)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)
![OBS Studio](https://img.shields.io/badge/OBS%20Studio-30%2B%20%2F%2031%2B%20%2F%2033%2B-black)
![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C)

> **OBSMV** est un plugin natif pour **OBS Studio** (Windows, Linux, macOS) offrant à la fois un **visualiseur audio de pointe** (à la manière de *Spectralizer*) et un affichage personnalisable de la musique en cours (**Now Playing**) rendu **directement par le moteur graphique d'OBS (GPU)**, sans aucune source web ni navigateur (CEF) intermédiaire.

---

## Points Forts & Fonctionnalités

### 1. Rendu 100% Interne & Natif dans OBS Studio
- **Zéro source web (Browser Source)** : Rendu matériel direct via le pipeline graphique OBS (`gs_*`).
- **Empreinte minimale** : Moins de 1% d'utilisation CPU/GPU, aucune dépendance Chromium.
- **Typographie nette et vectorielle** : Rasterisation haute fidélité via `stb_truetype` avec support des polices système ou fichiers `.ttf`/`.otf` personnalisés (police moderne *Inter* incluse par défaut).
- **Pochettes d'album décodées en GPU** : Décodage direct des images (PNG, JPEG) en textures matérielles OBS avec support des coins arrondis et effet vinyle optionnel (rotation fluide en lecture).

### 2. Visualiseur Audio Intégré
- **Capture audio temps réel** : Capture directe sur n'importe quelle source audio OBS (audio du bureau, micro, lecteur média, etc.).
- **Moteur FFT intégré (KissFFT)** : Analyse spectrale FFT embarquée, sans dépendance FFTW3 externe à installer.
- **4 styles de visualisation** :
  - **Barres verticales** (classique avec largeur, espacement et hauteur ajustables)
  - **Barres symétriques / centrées** (effet miroir)
  - **Onde / Oscillogramme** (spectre relié avec dégradé doux)
  - **Circulaire / Radial** (parfait autour d'une pochette d'album ou vinyle)
- **Lissage organique** : Filtre d'inertie type *Monstercat*, gravité et retombée fluide des crêtes.
- **Couleur dynamique** : Teinte automatiquement les barres du visualiseur avec la **couleur dominante extraite de la pochette d'album** !

### 3. Informations Musicales & Carte "Now Playing"
- **Mode Hybride intelligent** :
  - **Autonome natif** : Écoute directe des APIs système de transport média (**Windows SMTC**, **Linux D-Bus MPRIS**, **macOS**).
  - **Synchronisation OSMV** : Compatible avec le daemon [OSMV](https://github.com/streammusicviewer/OSMV) via IPC (`\\.\pipe\osmv-ipc-v3`) ou par lecture directe de `current_song.json`.
- **Éléments personnalisables** :
  - Pochette d'album (carrée, arrondie, ou disque vinyle avec rotation en lecture).
  - Titre, Artiste, Album avec templates de texte configurables (`{title}`, `{artist}`, `{album}`).
  - Barre de progression du morceau avec temps écoulé / durée totale (`01:45 / 03:20`).
  - Carte d'arrière-plan avec style verre fumé (Glassmorphism), couleurs et bordures éditables.

### 4. Trois Sources OBS Enregistrées
Vous pouvez utiliser OBSMV de la manière qui vous convient le mieux dans vos scènes OBS :
1. **`OBSMV (Visualizer + Now Playing)`** : Source tout-en-un réunissant le visualiseur et la carte de musique (avec choix de disposition : visualiseur au-dessus, en-dessous, et espacement).
2. **`OBSMV Visualizer`** : Source dédiée au visualiseur audio seul.
3. **`OBSMV Now Playing`** : Source dédiée à la carte musicale seule.

---

## Structure du Projet

```
OBSMV/
├── CMakeLists.txt              # Configuration CMake multiplateforme
├── data/
│   ├── fonts/
│   │   └── Inter.ttf           # Police moderne embarquée
│   └── locale/
│       ├── en-US.ini           # Traductions anglaises
│       └── fr-FR.ini           # Traductions françaises
├── deps/
│   └── libobs/                 # En-têtes et bibliothèque d'import OBS Studio
├── src/
│   ├── obsmv_plugin.cpp        # Point d'entrée du plugin OBS Studio
│   ├── media/                  # Détection SMTC / MPRIS / OSMV Sync
│   │   ├── media_detector.cpp
│   │   ├── media_osmv_sync.cpp
│   │   └── media_win_smtc.cpp
│   ├── render/                 # Rendu graphique natif OBS (texte, images, carte)
│   │   ├── card_renderer.cpp
│   │   ├── image_renderer.cpp
│   │   └── text_renderer.cpp
│   ├── source/                 # Implémentation des 3 sources OBS
│   │   ├── obsmv_source_combined.cpp
│   │   ├── obsmv_source_nowplaying.cpp
│   │   └── obsmv_source_visualizer.cpp
│   ├── third_party/            # KissFFT, stb_image, stb_truetype, json
│   └── visualizer/             # Capture audio OBS & processeur FFT
│       ├── audio_capture.cpp
│       ├── fft_processor.cpp
│       └── visualizer_renderer.cpp
├── windows/
│   └── build.bat               # Script de compilation 1-clic pour Windows
├── linux/
│   └── build.sh                # Script de compilation pour Linux
└── .github/workflows/
    └── build.yml               # Intégration continue CI multiplateforme (Win, Linux, macOS)
```

---

## Compilation

### Windows (Visual Studio 2022/2026 + CMake + Ninja)
Un script de build automatique est fourni :
```cmd
windows\build.bat
```
Le fichier binaire `obsmv.dll` sera généré dans `OBSMV\bin\obsmv.dll`.

### Linux (Ubuntu / Debian / Arch)
```bash
# Dépendances (Ubuntu/Debian)
sudo apt update && sudo apt install cmake ninja-build gcc g++ pkg-config libdbus-1-dev libobs-dev

# Compilation
./linux/build.sh
```
Le binaire `obsmv.so` sera généré dans `OBSMV/bin/obsmv.so`.

### macOS
```bash
cmake -B macos/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build macos/build -j
```

---

## Installation dans OBS Studio

1. **Copiez le binaire du plugin** :
   - **Windows** : Copiez `bin\obsmv.dll` vers `C:\Program Files\obs-studio\obs-plugins\64bit\`
   - **Linux** : Copiez `bin/obsmv.so` vers `~/.config/obs-studio/plugins/obsmv/bin/64bit/`
   - **macOS** : Copiez `bin/obsmv.so` vers `~/Library/Application Support/obs-studio/plugins/obsmv/bin/`
2. **Copiez les données (traductions & polices)** :
   - Copiez le dossier `data/` dans le répertoire de données du plugin OBS (ex. `C:\Program Files\obs-studio\data\obs-plugins\obsmv\`).
3. Lancez OBS Studio : ajoutez une source **OBSMV** dans votre scène !

---

## Licence

Distribué sous licence GNU General Public License v3.0 (GPL-3.0) — Voir [LICENSE](LICENSE) pour plus de détails.
