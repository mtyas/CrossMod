#!/usr/bin/env bash
set -e

# CrossMod macOS Build Script
# Builds Universal Binary (Apple Silicon arm64 + Intel x86_64)
# Formats generated: VST3, CLAP, AU (AudioUnit), and Standalone App

echo "=========================================================="
echo "  Building CrossMod for macOS (Universal Binary)          "
echo "=========================================================="

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$SCRIPT_DIR/.."
cd "$PROJECT_ROOT"

BUILD_DIR="build-mac"

# 1. Configure CMake for Universal Binary
echo ">> Configuring CMake for macOS (arm64 + x86_64)..."
cmake -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"

# 2. Compile in Parallel
echo ">> Compiling CrossMod plugins, standalone app & tests..."
cmake --build "$BUILD_DIR" --config Release --target CrossMod_All CrossMod_CLAP CrossModTests --parallel

# 3. Run Automated Tests
echo ">> Running unit verification tests..."
"$BUILD_DIR/CrossModTests"

# 4. Display Outputs
echo "=========================================================="
echo "  macOS Build Complete! Output files:                    "
echo "=========================================================="
echo "VST3:       $BUILD_DIR/CrossMod_artefacts/Release/VST3/CrossMod.vst3"
echo "CLAP:       $BUILD_DIR/CrossMod_artefacts/Release/CLAP/CrossMod.clap"
echo "AudioUnit:  $BUILD_DIR/CrossMod_artefacts/Release/AU/CrossMod.component"
echo "Standalone: $BUILD_DIR/CrossMod_artefacts/Release/Standalone/CrossMod.app"
echo "=========================================================="
