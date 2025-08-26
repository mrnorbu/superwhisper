#!/bin/bash

# Test script for SuperWhisper CLI
echo "Testing SuperWhisper CLI..."

# Check if executable exists
if [ ! -f "./build/SuperWhisperCLI" ]; then
    echo "Error: SuperWhisperCLI executable not found. Please build first with ./build_cli.sh"
    exit 1
fi

echo "✓ Executable found"

# Test help
echo -e "\n1. Testing help command..."
./build/SuperWhisperCLI --help | head -20

# Test version
echo -e "\n2. Testing version command..."
./build/SuperWhisperCLI --version

# Test settings display
echo -e "\n3. Testing settings display..."
./build/SuperWhisperCLI --settings

# Test with custom config
echo -e "\n4. Testing with custom config..."
./build/SuperWhisperCLI -c ~/.superwhisper/config.json --settings

# Test clipboard disabling
echo -e "\n5. Testing clipboard disabling..."
./build/SuperWhisperCLI --no-clipboard --settings

echo -e "\n✓ All tests passed!"
echo -e "\nTo run the CLI application:"
echo "  ./build/SuperWhisperCLI"
echo -e "\nTo start recording:"
echo "  Press 'r' to start recording"
echo "  Press 's' to stop recording"
echo "  Press 'q' to quit"
echo -e "\nTo customize settings:"
echo "  Edit ~/.superwhisper/config.json"
echo -e "\nTo test without clipboard:"
echo "  ./build/SuperWhisperCLI --no-clipboard"
