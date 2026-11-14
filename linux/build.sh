#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

echo "============================================================"
echo "Building OBSMV - OBS Studio Native Plugin for Linux"
echo "============================================================"

BUILD_DIR="linux/build"

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "[ERROR] cmake is required but not installed." >&2
    exit 1
fi

if ! command -v pkg-config &> /dev/null; then
    echo "[ERROR] pkg-config is required but not installed." >&2
    exit 1
fi

# Configure
mkdir -p "${BUILD_DIR}"
cmake -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build "${BUILD_DIR}" -j"$(nproc 2>/dev/null || echo 4)"

# Copy artifact
mkdir -p bin
cp -f "${BUILD_DIR}/obsmv.so" bin/obsmv.so 2>/dev/null || cp -f "${BUILD_DIR}/libobsmv.so" bin/obsmv.so

echo ""
echo "============================================================"
echo "[SUCCESS] OBSMV Plugin built successfully!"
echo "Binary: bin/obsmv.so"
echo "============================================================"

