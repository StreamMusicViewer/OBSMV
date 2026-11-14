# OBSMV (OBS Stream Music Visualizer)

![Status](https://img.shields.io/badge/status-active-success)
![Version](https://img.shields.io/badge/version-v1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)
![OBS Studio](https://img.shields.io/badge/OBS%20Studio-30%2B%20%2F%2031%2B%20%2F%2033%2B-black)
![C++20](https://img.shields.io/badge/Language-C%2B%2B20-00599C)

> **OBSMV** est un plugin natif pour **OBS Studio** (Windows, Linux, macOS) offrant à la fois un **visualiseur audio de pointe** (à la manière de *Spectralizer*) et un affichage personnalisable de la musique en cours (**Now Playing**) rendu **directement par le moteur graphique d'OBS (GPU)**, sans aucune source web ni navigateur (CEF) intermédiaire.

---

## Points Forts & Fonctionnalités

### 1. Rendu 100% Interne & Natif dans OBS Studio
- **Zéro source web (Browser Source)** : Rendu matériel direct via le pipeline graphique OBS (`gs_*`).
- **Empreinte minimale** : Moins de 1% d'utilisation CPU/GPU, aucune dépendance Chromium.
- **Typographie nette et vectorielle** : Rasterisation haute fidélité via `stb_truetype` avec supersampling 2x et gestion des polices système ou fichiers `.ttf`/`.otf` personnalisés.
- **Sélecteur de polices système intégré** : Choisissez directement n'importe quelle police installée sur votre ordinateur (Segoe UI, Arial, Roboto, Inter, Bahnschrift, etc.) via le sélecteur natif d'OBS.
- **Pochettes d'album décodées en GPU** : Décodage direct des images en textures matérielles OBS avec support des coins arrondis et effet vinyle optionnel (rotation fluide en lecture).

### 2. Visualiseur Audio Intégré (KissFFT)
- **Capture audio temps réel** : Capture directe sur n'importe quelle source audio OBS (audio du bureau, micro, lecteur média, etc.).
- **Moteur FFT intégré (KissFFT)** : Analyse spectrale FFT embarquée, sans dépendance externe à installer.
- **4 styles de visualisation** :
  - **Barres verticales** (largeur, espacement et hauteur ajustables)
  - **Barres symétriques / centrées** (effet miroir)
  - **Onde / Oscillogramme** (spline cubique de Catmull-Rom fluide à 12 sous-divisions, parfaitement calée sur la base)
  - **Circulaire / Radial** (idéal autour d'une pochette ou d'un disque vinyle)
- **Lissage organique** : Filtre d'inertie type *Monstercat*, gravité et retombée fluide des crêtes.
- **Ligne de crête lumineuse** : Contour lumineux suivant précisément la courbe de l'onde.

### 3. Couleur Dynamique Bicolore & Thèmes Dark / Light
- **Double palette intelligente** :
  - **Couleur d'accent vive** (pour les crêtes de l'onde, barres de progression et reflets)
  - **Couleur principale dominante** (pour le fond de carte et la base de l'onde)
- **Dégradé vertical** : Transition douce entre la couleur d'accent au sommet et la teinte de fond à la base.
- **Contrôle d'influence (style Photoshop)** : Dosage continu de l'influence de la pochette sur les couleurs personnalisées (de 0% à 100%).
- **Mode de thème** :
  - **Automatique (`Auto`)** : Détection de la luminance perçue de l'artwork.
  - **Sombre (`Dark Glass`)** : Fond verre fumé teinté avec texte blanc pur haute lisibilité.
  - **Clair (`Frosted Light`)** : Fond dépoli clair avec typographie sombre à fort contraste WCAG.
- **Bordures nettes** : Rendu des 4 bordures sans aucun artefact de superposition dans les coins.

### 4. Informations Musicales & Carte "Now Playing"
- **Mode Hybride intelligent** :
  - **Autonome natif** : Écoute directe des APIs système (**Windows SMTC**, **Linux D-Bus MPRIS**, **macOS**).
  - **Synchronisation OSMV** : Compatible avec le daemon [OSMV](https://github.com/StreamMusicViewer/OSMV) via IPC (`\\.\pipe\osmv-ipc-v3`) ou par lecture directe de `current_song.json`.
- **Éléments personnalisables** :
  - Pochette d'album (carrée, arrondie, ou vinyle rotatif).
  - Titre, Artiste, Album avec templates configurables (`{title}`, `{artist}`, `{album}`).
  - Barre de progression avec temps écoulé / durée totale (`01:45 / 03:20`).
  - Carte d'arrière-plan avec style verre (Glassmorphism), couleurs, opacité et bordures éditables.

### 5. Trois Sources OBS Enregistrées
1. **`OBSMV (Visualizer + Now Playing)`** : Source tout-en-un réunissant le visualiseur et la carte de musique (avec disposition au choix : visualiseur en haut ou en bas, espacement ajustable).
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
├── release/                    # Fichiers prêts pour release GitHub (v1.0.0)
│   ├── obsmv-v1.0.0-windows-x64.zip
│   ├── obsmv-v1.0.0-windows-x64.zip.sha256
│   ├── install-windows.bat
│   └── RELEASE_NOTES_v1.0.0.md
├── src/
│   ├── obsmv_plugin.cpp        # Point d'entrée du plugin OBS Studio
│   ├── media/                  # Détection SMTC / MPRIS / OSMV Sync
│   │   ├── media_detector.cpp
│   │   ├── media_linux_mpris.cpp
│   │   ├── media_osmv_sync.cpp
│   │   └── media_win_smtc.cpp
│   ├── render/                 # Rendu graphique natif OBS (texte, images, carte)
│   │   ├── card_renderer.cpp
│   │   ├── color_utils.hpp
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
│   ├── build.bat               # Script de compilation Windows (Ninja/MSVC)
│   └── install.bat             # Script d'installation locale dans OBS Studio
├── linux/
│   └── build.sh                # Script de compilation Linux
└── .github/workflows/
    └── build.yml               # CI multiplateforme (Win x64, Linux x86_64, macOS Universal)
```

---

## Installation Rapide

### Windows (64-bit)
1. Rendez-vous sur la page des **[Releases](https://github.com/StreamMusicViewer/OBSMV/releases)** et téléchargez `obsmv-v1.0.0-windows-x64.zip`.
2. Extrayez l'archive, fermez OBS Studio, et lancez `install.bat`.
3. Relancez OBS Studio et ajoutez la source **OBSMV** !

### Linux
```bash
# Dépendances (Ubuntu/Debian)
sudo apt update && sudo apt install cmake ninja-build gcc g++ pkg-config libdbus-1-dev

# Compilation
./linux/build.sh
```
Copiez `bin/obsmv.so` vers `~/.config/obs-studio/plugins/obsmv/bin/64bit/` et `data/` vers `~/.config/obs-studio/plugins/obsmv/data/`.

### macOS (Universal arm64 + x86_64)
```bash
brew install ninja simde
cmake -B macos/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build macos/build -j
```
Copiez `macos/build/obsmv.so` vers `~/Library/Application Support/obs-studio/plugins/obsmv/bin/`.

---

## Licence

Distribué sous licence GNU General Public License v3.0 (GPL-3.0) — Voir [LICENSE](LICENSE) pour plus de détails.
Développé par [@Ulyxx3](https://github.com/Ulyxx3) pour l'organisation [StreamMusicViewer](https://github.com/StreamMusicViewer).
