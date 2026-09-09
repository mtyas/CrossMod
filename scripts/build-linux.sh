#!/usr/bin/env bash
set -e

# CrossMod Linux Build Script
# Installs development dependencies (Debian/Ubuntu) and compiles VST3, CLAP, and Standalone.

echo "=========================================================="
echo "  Building CrossMod for Linux (x86_64)                    "
echo "=========================================================="

# Check for required packages on Debian/Ubuntu systems
if command -v apt-get &> /dev/null; then
  echo ">> Checking Linux audio & GUI development dependencies..."
  sudo apt-get update -qq
  sudo apt-get install -y -qq \
    build-essential \
    cmake \
    libasound2-dev \
    libjack-jackd2-dev \
    libgl1-mesa-dev \
    libx11-dev \
    libxcomposite-dev \
    libxcursor-dev \
    libxinerama-dev \
    libxrandr-dev \
    libxrender-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    libcurl4-openssl-dev \
    libgtk-3-dev
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
cd "$PROJECT_ROOT"

BUILD_DIR="build-linux"

# 1. Configure CMake
echo ">> Configuring CMake for Linux..."
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release

# 2. Compile in Parallel
echo ">> Compiling CrossMod plugins, standalone app & tests..."
cmake --build "$BUILD_DIR" --config Release --target CrossMod_All CrossMod_CLAP CrossModTests --parallel

# 3. Run Automated Tests
echo ">> Running unit verification tests..."
"$BUILD_DIR/CrossModTests"

# 4. Display Outputs
echo "=========================================================="
echo "  Linux Build Complete! Output files:                     "
echo "=========================================================="
echo "VST3:       $BUILD_DIR/CrossMod_artefacts/Release/VST3/CrossMod.vst3"
echo "CLAP:       $BUILD_DIR/CrossMod_artefacts/Release/CLAP/CrossMod.clap"
echo "Standalone: $BUILD_DIR/CrossMod_artefacts/Release/Standalone/CrossMod"
echo "=========================================================="
