#!/bin/bash

# SuperWhisper CLI Quick Start Script
echo "🚀 SuperWhisper CLI Quick Start"
echo "================================"

# Check if we're on macOS
if [[ "$OSTYPE" != "darwin"* ]]; then
    echo "❌ This script is designed for macOS only"
    exit 1
fi

echo "✅ macOS detected"

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "📦 Installing Homebrew..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
else
    echo "✅ Homebrew already installed"
fi

# Install dependencies
echo "📦 Installing dependencies..."
brew install portaudio nlohmann-json cmake

# Build the application
echo "🔨 Building SuperWhisper CLI..."
chmod +x build_cli.sh
./build_cli.sh

# Create config directory and copy example
echo "⚙️  Setting up configuration..."
mkdir -p ~/.superwhisper
cp config.json.example ~/.superwhisper/config.json

# Test the build
echo "🧪 Testing the build..."
chmod +x test_cli.sh
./test_cli.sh

echo ""
echo "🎉 Setup complete!"
echo ""
echo "Quick Commands:"
echo "  ./build/SuperWhisperCLI          # Start the application"
echo "  ./build/SuperWhisperCLI --help   # Show help"
echo "  ./build/SuperWhisperCLI --settings # Show current settings"
echo ""
echo "Usage:"
echo "  1. Run: ./build/SuperWhisperCLI"
echo "  2. Press 'r' to start recording"
echo "  3. Speak into your microphone"
echo "  4. Press 's' to stop recording"
echo "  5. View transcription result"
echo "  6. Press 'q' to quit"
echo ""
echo "Configuration:"
echo "  Edit ~/.superwhisper/config.json to customize settings"
echo ""
echo "Happy transcribing! 🎤✨"
