#pragma once

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <optional>
#include <chrono>

// Forward declarations
struct GLFWwindow;
struct whisper_context;

namespace SuperWhisper {

// Audio sample type - using 16-bit for memory efficiency
using AudioSample = int16_t;
using AudioBuffer = std::vector<AudioSample>;

// App states
enum class AppState {
    Ready,
    Recording,
    Transcribing,
    Error
};

// Audio recorder interface
class AudioRecorder {
public:
    virtual ~AudioRecorder() = default;
    
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool is_recording() const = 0;
    virtual AudioBuffer get_audio() const = 0;
    virtual void clear() = 0;
    
    // Memory-efficient streaming interface
    virtual void set_audio_callback(std::function<void(const AudioSample*, size_t)> callback) = 0;
};

// Whisper wrapper interface
class WhisperWrapper {
public:
    virtual ~WhisperWrapper() = default;
    
    virtual bool load_model(const std::string& path) = 0;
    virtual std::string transcribe(const AudioBuffer& audio, int sample_rate) = 0;
    virtual bool is_loaded() const = 0;
    
    // Memory management
    virtual void unload_model() = 0;
    virtual size_t get_memory_usage() const = 0;
};

// GUI manager interface
class GuiManager {
public:
    virtual ~GuiManager() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void render() = 0;
    virtual bool should_close() const = 0;
    
    // State updates
    virtual void set_state(AppState state) = 0;
    virtual void set_status(const std::string& status) = 0;
    virtual void set_hint(const std::string& hint) = 0;
    
    // Window management
    virtual void set_position(int x, int y) = 0;
    virtual void get_position(int& x, int& y) const = 0;
    virtual void set_always_on_top(bool enabled) = 0;
    
    // Button callback
    virtual void set_button_callback(std::function<void()> callback) = 0;
};

// Hotkey manager interface
class HotkeyManager {
public:
    virtual ~HotkeyManager() = default;
    
    virtual bool register_hotkey(int key_code) = 0;
    virtual void unregister_hotkey() = 0;
    virtual void set_callback(std::function<void()> callback) = 0;
};

// Main application class
class SuperWhisperApp {
public:
    SuperWhisperApp();
    ~SuperWhisperApp();
    
    // Main lifecycle
    bool initialize();
    void run();
    void shutdown();
    
    // State management
    void set_state(AppState state);
    AppState get_state() const;
    
    // Audio control
    void start_recording();
    void stop_recording();
    
    // Settings
    void save_settings();

private:
    // Core components
    std::unique_ptr<AudioRecorder> audio_recorder_;
    std::unique_ptr<WhisperWrapper> whisper_wrapper_;
    std::unique_ptr<GuiManager> gui_manager_;
    std::unique_ptr<HotkeyManager> hotkey_manager_;
    
    // App state
    std::atomic<AppState> state_{AppState::Ready};
    std::atomic<bool> is_recording_{false};
    std::atomic<bool> should_exit_{false};
    
    // Audio processing
    AudioBuffer audio_buffer_;
    std::mutex audio_mutex_;
    std::condition_variable audio_cv_;
    
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
    void auto_paste_text(const std::string& text);
    
    // Memory management
    void cleanup_resources();
    void optimize_memory_usage();
    
    // Error handling
    void handle_error(const std::string& error);
    void recover_from_error();
    
    // Hotkey callback
    void on_hotkey_pressed();
    
    // GUI callbacks
    void on_gui_button_click();
    void on_gui_close();
};

} // namespace SuperWhisper
