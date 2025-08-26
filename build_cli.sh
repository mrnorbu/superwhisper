#!/bin/bash

# SuperWhisper CLI Build Script
# This script builds the command-line interface version

set -e

echo "Building SuperWhisper CLI..."

# Check if we're on macOS
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS"
    
    # Check if Homebrew is installed
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found. Installing..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    
    # Install dependencies
    echo "Installing dependencies..."
    brew install portaudio nlohmann-json cmake
    
    # Check if nlohmann-json is available
    if ! pkg-config --exists nlohmann_json; then
        echo "nlohmann-json not found via pkg-config, installing manually..."
        brew install nlohmann-json
    fi
    
else
    echo "This script is designed for macOS. Please install dependencies manually:"
    echo "- PortAudio"
    echo "- nlohmann/json"
    echo "- CMake"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Build complete! Executable: build/SuperWhisperCLI"
echo ""
echo "Usage:"
echo "  ./SuperWhisperCLI --help"
echo "  ./SuperWhisperCLI -c ~/.superwhisper/config.json"
echo ""
echo "To create a default config:"
echo "  mkdir -p ~/.superwhisper"
echo "  cp ../config.json.example ~/.superwhisper/config.json"
