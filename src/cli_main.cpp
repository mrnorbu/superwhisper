#include "settings.hpp"
#include "audio_recorder.hpp"
#include "whisper_wrapper.hpp"
#include "hotkey_manager.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <termios.h> // Required for termios
#include <sys/ioctl.h> // Required for TIOCSTI

namespace SuperWhisper {

// Global flag for graceful shutdown
static std::atomic<bool> g_should_exit{false};

// Global terminal settings for restoration
static struct termios g_old_termios;
static bool g_terminal_modified = false;

// Cleanup function to restore terminal settings
void cleanup_terminal() {
    if (g_terminal_modified) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
        g_terminal_modified = false;
    }
}

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived shutdown signal, stopping gracefully..." << std::endl;
        
        // Restore terminal settings if modified
        cleanup_terminal();
        
        g_should_exit = true;
    }
}

// CLI application class
class SuperWhisperCLI {
public:
    SuperWhisperCLI() = default;
    ~SuperWhisperCLI() = default;
    
    bool initialize(const Settings& settings);
    void run();
    void shutdown();
    
    // Audio control
    void start_recording();
    void stop_recording();
    
    // Settings management
    void save_config(const std::string& path);
    void load_config(const std::string& path);
    
private:
    // Core components
    std::unique_ptr<AudioRecorder> audio_recorder_;
    std::unique_ptr<WhisperWrapper> whisper_wrapper_;
    std::unique_ptr<HotkeyManager> hotkey_manager_;
    
    // App state
    std::atomic<bool> is_recording_{false};
    Settings settings_;
    
    // Audio processing
    AudioBuffer audio_buffer_;
    std::mutex audio_mutex_;
    
    // Voice activity detection
    std::optional<std::chrono::steady_clock::time_point> last_voice_time_;
    
    // Worker threads
    std::thread recording_thread_;
    std::thread transcription_thread_;
    
    // Internal methods
    void recording_worker();
    void transcription_worker();
    void process_audio_chunk(const AudioSample* data, size_t count);
    void handle_transcription_result(const std::string& text);
    void copy_to_clipboard(const std::string& text);
    void save_to_file(const std::string& text);
    
    // Error handling
    void handle_error(const std::string& error);
    
    // Hotkey callbacks
    void on_hotkey_start();
    void on_hotkey_stop();
    void on_hotkey_quit();
};

bool SuperWhisperCLI::initialize(const Settings& settings) {
    try {
        settings_ = settings;
        
        // Initialize audio recorder
        audio_recorder_ = create_audio_recorder();
        if (!audio_recorder_) {
            std::cerr << "Failed to create audio recorder" << std::endl;
            return false;
        }
        
        // Initialize Whisper wrapper
        whisper_wrapper_ = create_whisper_wrapper();
        if (!whisper_wrapper_) {
            std::cerr << "Failed to create Whisper wrapper" << std::endl;
            return false;
        }
        
        // Initialize hotkey manager if enabled
        if (settings_.enable_hotkeys) {
            // Check input mode settings
            if (settings_.enable_global_hotkeys) {
                hotkey_manager_ = create_hotkey_manager();
                if (hotkey_manager_ && hotkey_manager_->is_supported()) {
                    if (hotkey_manager_->initialize()) {
                        // Register hotkeys
                        hotkey_manager_->register_start_hotkey(settings_.start_hotkey, 
                            [this]() { on_hotkey_start(); });
                        hotkey_manager_->register_stop_hotkey(settings_.stop_hotkey, 
                            [this]() { on_hotkey_stop(); });
                        hotkey_manager_->register_quit_hotkey(settings_.quit_hotkey, 
                            [this]() { on_hotkey_quit(); });
                        
                        std::cout << "Global hotkeys registered: " << settings_.start_hotkey 
                                  << " (start), " << settings_.stop_hotkey 
                                  << " (stop), " << settings_.quit_hotkey << " (quit)" << std::endl;
                    } else {
                        std::cout << "Warning: Failed to initialize global hotkeys" << std::endl;
                    }
                } else {
                    std::cout << "Warning: Global hotkeys not supported on this platform" << std::endl;
                }
            }
            
            if (settings_.enable_terminal_input) {
                std::cout << "Terminal input enabled: Use 'r' (start), 's' (stop), 'q' (quit)" << std::endl;
            }
        }
        
        // Load Whisper model
        if (!whisper_wrapper_->load_model(settings_.model_path)) {
            std::cerr << "Failed to load Whisper model: " << settings_.model_path << std::endl;
            return false;
        }
        
        // Set up audio callback for real-time processing
        audio_recorder_->set_audio_callback([this](const AudioSample* data, size_t count) {
            process_audio_chunk(data, count);
        });
        
        std::cout << "SuperWhisper CLI initialized successfully" << std::endl;
        std::cout << "Model loaded: " << settings_.model_path << std::endl;
        std::cout << "Press Ctrl+C to exit" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void SuperWhisperCLI::run() {
    std::cout << "Starting SuperWhisper CLI..." << std::endl;
    
    // Show appropriate input instructions based on settings
    if (settings_.enable_terminal_input) {
        std::cout << "Press 'r' to start recording, 's' to stop, 'q' to quit" << std::endl;
    }
    if (settings_.enable_global_hotkeys && hotkey_manager_ && hotkey_manager_->is_supported()) {
        std::cout << "Global hotkeys: " << settings_.start_hotkey << " (start), " 
                  << settings_.stop_hotkey << " (stop), " << settings_.quit_hotkey << " (quit)" << std::endl;
    }
    
    // Set terminal to non-canonical mode for immediate key input (only if terminal input enabled)
    struct termios new_termios;
    bool terminal_mode_enabled = false;
    
    if (settings_.enable_terminal_input) {
        tcgetattr(STDIN_FILENO, &g_old_termios);
        g_terminal_modified = true;
        new_termios = g_old_termios;
        new_termios.c_lflag &= ~(ICANON | ECHO);
        new_termios.c_cc[VMIN] = 0;
        new_termios.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
        terminal_mode_enabled = true;
    }
    
    char input;
    while (!g_should_exit) {
        // Non-blocking input check (only if terminal input enabled)
        if (settings_.enable_terminal_input && read(STDIN_FILENO, &input, 1) > 0) {
            switch (input) {
                case 'r':
                case 'R':
                    if (!is_recording_) {
                        start_recording();
                    }
                    break;
                    
                case 's':
                case 'S':
                    if (is_recording_) {
                        stop_recording();
                    }
                    break;
                    
                case 'q':
                case 'Q':
                    g_should_exit = true;
                    break;
                    
                case '\n':
                    // Ignore newline
                    break;
                    
                default:
                    if (is_recording_) {
                        std::cout << "Recording... Press 's' to stop" << std::endl;
                    } else if (settings_.enable_terminal_input) {
                        std::cout << "Press 'r' to start recording, 's' to stop, 'q' to quit" << std::endl;
                    }
                    break;
            }
        }
        
        // Check for silence detection if recording
        if (is_recording_) {
            auto now = std::chrono::steady_clock::now();
            if (last_voice_time_ && (now - *last_voice_time_) > std::chrono::duration<float>(settings_.silence_duration)) {
                std::cout << "Silence detected, stopping recording..." << std::endl;
                stop_recording();
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Reduced sleep for more responsive input
    }
    
    // Restore terminal settings if modified
    if (g_terminal_modified) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
        g_terminal_modified = false;
    }
}

void SuperWhisperCLI::shutdown() {
    // Stop recording
    stop_recording();
    
    // Wait for worker threads
    if (recording_thread_.joinable()) {
        recording_thread_.join();
    }
    if (transcription_thread_.joinable()) {
        transcription_thread_.join();
    }
    
    // Cleanup components
    if (audio_recorder_) audio_recorder_->stop();
    if (whisper_wrapper_) whisper_wrapper_->unload_model();
    if (hotkey_manager_) hotkey_manager_->shutdown();
    
    // Restore terminal settings if modified
    if (g_terminal_modified) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
        g_terminal_modified = false;
    }
    
    std::cout << "SuperWhisper CLI shutdown complete" << std::endl;
}

void SuperWhisperCLI::start_recording() {
    if (is_recording_) return;
    
    try {
        // Ensure previous threads are cleaned up
        if (recording_thread_.joinable()) {
            recording_thread_.join();
        }
        if (transcription_thread_.joinable()) {
            transcription_thread_.join();
        }
        
        // Clear any previous audio data
        if (audio_recorder_) {
            audio_recorder_->clear();
        }
        
        if (!audio_recorder_->start()) {
            handle_error("Failed to start recording");
            return;
        }
        
        is_recording_ = true;
        std::cout << "Recording started... (Press 's' to stop)" << std::endl;
        
        // Start recording worker thread
        recording_thread_ = std::thread([this]() { recording_worker(); });
        
        // Reset voice detection
        last_voice_time_ = std::chrono::steady_clock::now();
        
    } catch (const std::exception& e) {
        handle_error("Recording start failed: " + std::string(e.what()));
    }
}

void SuperWhisperCLI::stop_recording() {
    if (!is_recording_) return;
    
    is_recording_ = false;
    
    if (audio_recorder_) {
        audio_recorder_->stop();
    }
    
    // Wait for recording thread to finish
    if (recording_thread_.joinable()) {
        recording_thread_.join();
    }
    
    // Wait for any previous transcription thread to complete
    if (transcription_thread_.joinable()) {
        transcription_thread_.join();
    }
    
    // Check if we have audio to transcribe
    if (audio_recorder_ && !audio_recorder_->get_audio().empty()) {
        std::cout << "Transcribing audio..." << std::endl;
        
        // Start transcription in separate thread
        transcription_thread_ = std::thread([this]() { transcription_worker(); });
    } else {
        std::cout << "No audio recorded" << std::endl;
    }
}

void SuperWhisperCLI::recording_worker() {
    try {
        auto start_time = std::chrono::steady_clock::now();
        
        while (is_recording_) {
            // Check duration limit
            auto now = std::chrono::steady_clock::now();
            if ((now - start_time) > std::chrono::seconds(settings_.max_duration)) {
                std::cout << "Maximum duration reached, stopping recording..." << std::endl;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Recording worker error: " << e.what() << std::endl;
    }
}

void SuperWhisperCLI::transcription_worker() {
    try {
        if (!whisper_wrapper_ || !audio_recorder_) {
            handle_error("Transcription components not available");
            return;
        }
        
        // Get recorded audio
        AudioBuffer audio = audio_recorder_->get_audio();
        if (audio.empty()) {
            handle_error("No audio to transcribe");
            return;
        }
        
        // Transcribe audio
        std::string text = whisper_wrapper_->transcribe(audio, settings_.sample_rate, settings_);
        
        if (!text.empty()) {
            handle_transcription_result(text);
        } else {
            handle_error("Transcription produced no text");
        }
        
        // Clear audio buffer to free memory
        audio_recorder_->clear();
        
    } catch (const std::exception& e) {
        handle_error("Transcription failed: " + std::string(e.what()));
    }
}

void SuperWhisperCLI::process_audio_chunk(const AudioSample* data, size_t count) {
    if (!is_recording_) return;
    
    // Voice activity detection
    float max_amplitude = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float amplitude = std::abs(static_cast<float>(data[i])) / 32768.0f;
        max_amplitude = std::max(max_amplitude, amplitude);
    }
    
    if (max_amplitude > settings_.silence_threshold) {
        last_voice_time_ = std::chrono::steady_clock::now();
    }
}

void SuperWhisperCLI::handle_transcription_result(const std::string& text) {
    try {
        std::cout << "\n=== Transcription Result ===" << std::endl;
        std::cout << text << std::endl;
        std::cout << "===========================" << std::endl;
        
        // Save to file if specified
        if (!settings_.output_file.empty()) {
            save_to_file(text);
        }
        
        // Copy to clipboard if enabled
        if (settings_.copy_to_clipboard) {
            copy_to_clipboard(text);
        } else {
            std::cout << "Clipboard copying disabled in settings" << std::endl;
        }
        
        std::cout << "Press 'r' to start new recording, 'q' to quit" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Transcription handling error: " << e.what() << std::endl;
        handle_error("Failed to handle transcription: " + std::string(e.what()));
    }
}

void SuperWhisperCLI::copy_to_clipboard(const std::string& text) {
    // On macOS, use pbcopy to copy text to clipboard
    // Use a temporary file approach for better reliability with special characters
    
    try {
        // Create a temporary file
        std::string temp_file = "/tmp/superwhisper_clipboard_" + std::to_string(getpid()) + ".txt";
        
        // Write text to temporary file
        std::ofstream temp_out(temp_file);
        if (!temp_out.is_open()) {
            std::cout << "Failed to create temporary file for clipboard" << std::endl;
            return;
        }
        temp_out << text;
        temp_out.close();
        
        // Copy from file to clipboard
        std::string command = "cat '" + temp_file + "' | pbcopy";
        int result = system(command.c_str());
        
        // Clean up temporary file
        std::remove(temp_file.c_str());
        
        if (result == 0) {
            std::cout << "Text copied to clipboard" << std::endl;
        } else {
            std::cout << "Failed to copy to clipboard (error code: " << result << ")" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Failed to copy to clipboard: " << e.what() << std::endl;
    }
}

void SuperWhisperCLI::save_to_file(const std::string& text) {
    try {
        std::ofstream file(settings_.output_file);
        if (file.is_open()) {
            file << text << std::endl;
            file.close();
            std::cout << "Text saved to: " << settings_.output_file << std::endl;
        } else {
            std::cerr << "Failed to open output file: " << settings_.output_file << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to save to file: " << e.what() << std::endl;
    }
}

void SuperWhisperCLI::handle_error(const std::string& error) {
    std::cerr << "Error: " << error << std::endl;
}

void SuperWhisperCLI::on_hotkey_start() {
    start_recording();
}

void SuperWhisperCLI::on_hotkey_stop() {
    stop_recording();
}

void SuperWhisperCLI::on_hotkey_quit() {
    g_should_exit = true;
}

void SuperWhisperCLI::save_config(const std::string& path) {
    settings_.save(path);
}

void SuperWhisperCLI::load_config(const std::string& path) {
    settings_.load(path);
}

} // namespace SuperWhisper

// Main entry point
int main(int argc, char* argv[]) {
    try {
        // Set up signal handlers
        signal(SIGINT, SuperWhisper::signal_handler);
        signal(SIGTERM, SuperWhisper::signal_handler);
        
        // Register cleanup function for atexit
        atexit(SuperWhisper::cleanup_terminal);
        
        // Parse command line arguments
        std::string config_file = "~/.superwhisper/config.json";
        std::string model_path = "";
        bool show_help = false;
        bool show_settings = false;
        bool disable_clipboard = false;
        
        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                show_help = true;
            } else if (strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-c") == 0) {
                if (i + 1 < argc) {
                    config_file = argv[++i];
                }
            } else if (strcmp(argv[i], "--model") == 0 || strcmp(argv[i], "-m") == 0) {
                if (i + 1 < argc) {
                    model_path = argv[++i];
                }
            } else if (strcmp(argv[i], "--settings") == 0 || strcmp(argv[i], "-s") == 0) {
                show_settings = true;
            } else if (strcmp(argv[i], "--help-settings") == 0) {
                SuperWhisper::Settings settings;
                settings.load(config_file);
                settings.print_help();
                return 0;
            } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
                std::cout << "SuperWhisper CLI v1.0.0" << std::endl;
                return 0;
            } else if (strcmp(argv[i], "--no-clipboard") == 0) {
                disable_clipboard = true;
            }
        }
        
        if (show_help) {
            std::cout << "SuperWhisper CLI - Command Line Interface\n\n";
            std::cout << "Usage: superwhisper [options]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -h, --help           Show this help message\n";
            std::cout << "  -c, --config FILE    Specify config file (default: ~/.superwhisper/config.json)\n";
            std::cout << "  -m, --model PATH     Override model path from config\n";
            std::cout << "  -s, --settings       Show current settings\n";
            std::cout << "  --help-settings      Show all available settings with descriptions\n";
            std::cout << "  -v, --version        Show version information\n";
            std::cout << "  --no-clipboard       Disable clipboard copying for testing\n\n";
            std::cout << "Interactive Commands:\n";
            std::cout << "  r                    Start recording\n";
            std::cout << "  s                    Stop recording\n";
            std::cout << "  q                    Quit application\n\n";
            std::cout << "Global Hotkeys (configurable):\n";
            std::cout << "  F9                   Start recording (default)\n";
            std::cout << "  F10                  Stop recording (default)\n";
            std::cout << "  F12                  Quit application (default)\n\n";
            std::cout << "Example:\n";
            std::cout << "  superwhisper -c ~/myconfig.json -m /path/to/model.bin\n\n";
            std::cout << "Note: Hotkeys require accessibility permissions on macOS.\n";
            std::cout << "      Go to System Preferences > Security & Privacy > Accessibility\n";
            std::cout << "      and add Terminal (or your terminal app) to the list.\n\n";
            return 0;
        }
        
        // Load settings
        SuperWhisper::Settings settings;
        settings.load(config_file);
        
        // Override model path if specified
        if (!model_path.empty()) {
            settings.model_path = model_path;
        }

        // Disable clipboard copying if specified
        if (disable_clipboard) {
            settings.copy_to_clipboard = false;
            std::cout << "Clipboard copying disabled by command line option." << std::endl;
        }
        
        if (show_settings) {
            settings.print_current_settings();
            return 0;
        }
        
        // Create and run CLI application
        SuperWhisper::SuperWhisperCLI app;
        
        if (!app.initialize(settings)) {
            std::cerr << "Failed to initialize SuperWhisper CLI" << std::endl;
            return 1;
        }
        
        app.run();
        app.shutdown();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
