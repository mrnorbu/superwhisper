#!/bin/bash

# SuperWhisper C++ Test Script
# Tests the build and basic functionality

set -e

echo "🧪 Testing SuperWhisper C++ build..."

# Check if build exists
if [ ! -f "build/SuperWhisper" ]; then
    echo "❌ Build not found. Run ./build.sh first."
    exit 1
fi

echo "✅ Build found: build/SuperWhisper"

# Check file size
file_size=$(stat -f%z "build/SuperWhisper" 2>/dev/null || stat -c%s "build/SuperWhisper" 2>/dev/null)
echo "📏 Binary size: $((file_size / 1024 / 1024)) MB"

# Check architecture
echo "🏗️  Checking architecture..."
file build/SuperWhisper

# Check dependencies
echo "🔍 Checking dependencies..."
otool -L build/SuperWhisper 2>/dev/null || ldd build/SuperWhisper 2>/dev/null || echo "Could not check dependencies"

# Test model file
if [ -f "model/ggml-base.en-q5_1.bin" ]; then
    echo "✅ Whisper model found"
    model_size=$(stat -f%z "model/ggml-base.en-q5_1.bin" 2>/dev/null || stat -c%s "model/ggml-base.en-q5_1.bin" 2>/dev/null)
    echo "📏 Model size: $((model_size / 1024 / 1024)) MB"
else
    echo "⚠️  Whisper model not found at model/ggml-base.en-q5_1.bin"
    echo "   You can download it from: https://huggingface.co/ggerganov/whisper.cpp"
fi

# Check permissions
echo "🔐 Checking permissions..."
ls -la build/SuperWhisper

echo ""
echo "🎯 Build test complete!"
echo ""
echo "To run SuperWhisper:"
echo "  ./build/SuperWhisper"
echo ""
echo "To test with audio:"
echo "  1. Ensure microphone permissions are granted"
echo "  2. Run the application"
echo "  3. Click the green button or press F9"
echo "  4. Speak into your microphone"
echo "  5. Stop recording to test transcription"
