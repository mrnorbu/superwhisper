#!/bin/bash

# SuperWhisper C++ Build Script for macOS Apple Silicon
# This script sets up dependencies and builds the application

set -e

echo "🚀 Building SuperWhisper for macOS Apple Silicon..."

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ This script is designed for macOS only"
    exit 1
fi

# Check if we're on Apple Silicon
if [[ $(uname -m) != "arm64" ]]; then
    echo "⚠️  This script is optimized for Apple Silicon (M1/M2/M3). Intel Macs may work but are not optimized."
fi

# Check for required tools
command -v cmake >/dev/null 2>&1 || { echo "❌ cmake is required but not installed. Install with: brew install cmake"; exit 1; }
command -v brew >/dev/null 2>&1 || { echo "❌ Homebrew is required but not installed. Install from https://brew.sh"; exit 1; }

echo "📦 Installing dependencies..."

# Install system dependencies
brew install cmake pkg-config glfw portaudio

echo "🔧 Setting up external libraries..."

# Create external directory
mkdir -p external
cd external

# Clone Dear ImGui if not exists
if [ ! -d "imgui" ]; then
    echo "📥 Cloning Dear ImGui..."
    git clone https://github.com/ocornut/imgui.git
    cd imgui
    git checkout v1.90.1
    cd ..
else
    echo "✅ Dear ImGui already exists"
fi

# Clone whisper.cpp if not exists
if [ ! -d "whisper.cpp" ]; then
    echo "📥 Cloning whisper.cpp..."
    git clone https://github.com/ggerganov/whisper.cpp.git
    cd whisper.cpp
    git checkout v1.5.4
    cd ..
else
    echo "✅ whisper.cpp already exists"
fi

# Go back to project root
cd ..

echo "🏗️  Building SuperWhisper..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake for MAXIMUM PERFORMANCE
echo "📐 Configuring with CMake for maximum speed..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DWHISPER_METAL=ON \
    -DWHISPER_ACCELERATE=ON \
    -DGGML_NATIVE=ON \
    -DGGML_LTO=ON

# Build
echo "🔨 Building..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "✅ Build complete!"
echo ""
echo "🎯 To run SuperWhisper:"
echo "   ./SuperWhisper"
echo ""
echo "📝 Note: You may need to grant Accessibility permissions to allow global hotkeys:"
echo "   System Settings > Privacy & Security > Accessibility → Add SuperWhisper"
echo ""
echo "🔧 To clean build:"
echo "   make clean"
echo ""
echo "🗑️  To remove all build files:"
echo "   cd .. && rm -rf build"
