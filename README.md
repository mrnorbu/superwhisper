# SuperWhisper CLI

A high-performance command-line interface for real-time speech-to-text transcription using OpenAI's Whisper model, optimized for macOS with Metal GPU acceleration.

## 🚀 Features

- **Real-time Audio Recording**: Voice activity detection with configurable silence thresholds
- **Whisper Integration**: Full whisper.cpp integration with all configurable parameters
- **Multiple Output Formats**: Text, JSON, SRT, VTT, and CSV output formats
- **Global Hotkeys**: Configurable system-wide hotkeys for recording control
- **GPU Acceleration**: Metal GPU support on macOS for maximum performance
- **Comprehensive Configuration**: JSON-based configuration with extensive customization options

## 📋 Requirements

- **macOS**: 12.0+ (Apple Silicon recommended for best performance)
- **Homebrew**: For dependency management
- **CMake**: 3.20+ for building
- **Accessibility Permissions**: Required for global hotkeys

## 🛠️ Installation

### Quick Start
```bash
chmod +x quick_start.sh
./quick_start.sh
```

### Manual Installation
```bash
brew install portaudio nlohmann-json cmake
chmod +x build_cli.sh
./build_cli.sh
mkdir -p ~/.superwhisper
cp config.json.example ~/.superwhisper/config.json
```

## 🎯 Usage

### Basic Commands
```bash
./build/SuperWhisperCLI                    # Start with defaults
./build/SuperWhisperCLI --help            # Show help
./build/SuperWhisperCLI --settings        # Show current settings
./build/SuperWhisperCLI --help-settings   # Show all settings
./build/SuperWhisperCLI -c config.json    # Use custom config
./build/SuperWhisperCLI -m model.bin      # Override model path
```

### Interactive Commands
- `r` - Start recording
- `s` - Stop recording
- `q` - Quit application

### Global Hotkeys
- **F9** - Start recording (default)
- **F10** - Stop recording (default)
- **F12** - Quit application (default)

## ⚙️ Configuration

### Configuration File
`~/.superwhisper/config.json`

### Key Settings

#### Model Settings
```json
{
  "model_path": "model/ggml-base.en-q5_1.bin",
  "model_size": "base"
}
```

#### Audio Settings
```json
{
  "silence_duration": 1.0,
  "max_duration": 30,
  "silence_threshold": 0.01,
  "sample_rate": 16000
}
```

#### Whisper Settings
```json
{
  "language": "auto",
  "translate_to_english": false,
  "num_threads": 4,
  "max_tokens": 448,
  "temperature": 0.0,
  "print_timestamps": false,
  "print_progress": true
}
```

#### Output Settings
```json
{
  "output_format": "text",
  "output_file": "",
  "copy_to_clipboard": true
}
```

#### Hotkey Settings
```json
{
  "enable_hotkeys": true,
  "start_hotkey": "F9",
  "stop_hotkey": "F10",
  "quit_hotkey": "F12"
}
```

## 📊 Output Formats

- **text**: Plain text (default)
- **json**: Structured with timestamps
- **srt**: SubRip subtitle format
- **vtt**: WebVTT format
- **csv**: Comma-separated with timestamps

## 🔧 Technical Architecture

### Core Components
- **Audio Recorder**: PortAudio-based capture with voice activity detection
- **Whisper Wrapper**: whisper.cpp integration with GPU acceleration
- **Settings Manager**: JSON configuration with validation
- **Hotkey Manager**: Carbon framework integration for global hotkeys
- **CLI Interface**: Command-line parsing and interactive commands

### Performance Features
- Metal GPU acceleration on macOS
- Configurable CPU threading
- Memory-efficient audio processing
- Apple Silicon optimizations

## 🧪 Testing

```bash
./test_cli.sh              # Run all tests
./build_cli.sh             # Test build system
./build/SuperWhisperCLI -c test_config.json --settings
```

## 🔍 Troubleshooting

### Hotkeys Not Working
1. Check accessibility permissions in System Preferences
2. Verify `"enable_hotkeys": true` in config
3. Look for permission warnings in output

### Audio Issues
1. Check microphone permissions
2. Verify PortAudio installation
3. Adjust `silence_threshold` values

### Build Errors
1. Install dependencies: `brew install portaudio nlohmann-json cmake`
2. Ensure CMake 3.20+
3. Check macOS compatibility

## 📚 API Reference

### Settings Structure
```cpp
struct Settings {
    // Model, Audio, Whisper, Output, Performance, Hotkey settings
    std::string model_path;
    float silence_duration;
    std::string language;
    std::string output_format;
    bool enable_hotkeys;
    // ... and many more
};
```

### Factory Functions
```cpp
std::unique_ptr<AudioRecorder> create_audio_recorder();
std::unique_ptr<WhisperWrapper> create_whisper_wrapper();
std::unique_ptr<HotkeyManager> create_hotkey_manager();
```

## 🔮 Future Enhancements

- Cross-platform support (Linux, Windows)
- Batch processing capabilities
- HTTP/REST API integration
- Real-time streaming
- Custom model support
- Advanced hotkey combinations

## 📄 License

MIT License - see LICENSE file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

---

**SuperWhisper CLI** - High-performance, configurable speech-to-text transcription with global hotkey support.
