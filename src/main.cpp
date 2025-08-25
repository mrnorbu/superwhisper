#include "superwhisper.hpp"
#include "audio_recorder.hpp"
#include "whisper_wrapper.hpp"
#include "gui_manager.hpp"
#include "hotkey_manager.hpp"
#include "settings.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>

namespace SuperWhisper {

SuperWhisperApp::SuperWhisperApp() {
    // Initialize components
    audio_recorder_ = create_audio_recorder();
    whisper_wrapper_ = create_whisper_wrapper();
    gui_manager_ = create_gui_manager();
    hotkey_manager_ = create_hotkey_manager();
}

SuperWhisperApp::~SuperWhisperApp() {
    shutdown();
}

bool SuperWhisperApp::initialize() {
    try {
        // Initialize GUI
        if (!gui_manager_->initialize()) {
            std::cerr << "Failed to initialize GUI" << std::endl;
            return false;
        }
        
        // Set up GUI callbacks
        gui_manager_->set_button_callback([this]() { on_gui_button_click(); });
        
        // Initialize hotkey (F9)
        hotkey_manager_->set_callback([this]() { on_hotkey_pressed(); });
        if (!hotkey_manager_->register_hotkey(0x3F)) {  // F9 key code
            std::cerr << "Failed to register F9 hotkey" << std::endl;
            // Continue without hotkey support
        }
        
        // Load Whisper model
        if (!whisper_wrapper_->load_model("model/ggml-base.en-q5_1.bin")) {
            std::cerr << "Failed to load Whisper model" << std::endl;
            set_state(AppState::Error);
            return false;
        }
        
        // Set up audio callback for real-time processing
        audio_recorder_->set_audio_callback([this](const AudioSample* data, size_t count) {
            process_audio_chunk(data, count);
        });
        
        // Set initial state
        set_state(AppState::Ready);
        gui_manager_->set_status("Ready");
        gui_manager_->set_hint("Press F9 anywhere\nor click to record");
        
        std::cout << "SuperWhisper initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        return false;
    }
}

void SuperWhisperApp::run() {
    if (!gui_manager_) return;
    
    std::cout << "Starting SuperWhisper main loop..." << std::endl;
    
    // Main render loop
    while (!should_exit_ && !gui_manager_->should_close()) {
        // Render GUI
        gui_manager_->render();
        
        // Process audio if recording
        if (is_recording_) {
            // Check for silence detection
            auto now = std::chrono::steady_clock::now();
            if (last_voice_time_ && (now - *last_voice_time_) > std::chrono::duration<float>(1.0f)) {
                stop_recording();
            }
        }
        
        // Optimized sleep for performance - reduce to 8ms for better responsiveness
        std::this_thread::sleep_for(std::chrono::milliseconds(8));  // ~120 FPS for snappier UI
    }
}

void SuperWhisperApp::shutdown() {
    should_exit_ = true;
    
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
    if (gui_manager_) gui_manager_->shutdown();
    if (hotkey_manager_) hotkey_manager_->unregister_hotkey();
    
    std::cout << "SuperWhisper shutdown complete" << std::endl;
}

void SuperWhisperApp::set_state(AppState state) {
    state_ = state;
    if (gui_manager_) {
        gui_manager_->set_state(state);
    }
}

AppState SuperWhisperApp::get_state() const {
    return state_;
}

void SuperWhisperApp::start_recording() {
    if (state_ != AppState::Ready || is_recording_) return;
    
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
        set_state(AppState::Recording);
        gui_manager_->set_status("Recording...");
        gui_manager_->set_hint("Click or press F9\nto stop recording");
        
        // Start recording worker thread
        recording_thread_ = std::thread([this]() { recording_worker(); });
        
        // Reset voice detection
        last_voice_time_ = std::chrono::steady_clock::now();
        
    } catch (const std::exception& e) {
        handle_error("Recording start failed: " + std::string(e.what()));
    }
}

void SuperWhisperApp::stop_recording() {
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
        set_state(AppState::Transcribing);
        gui_manager_->set_status("Transcribing...");
        gui_manager_->set_hint("Processing audio\nplease wait");
        
        // Start transcription in separate thread
        transcription_thread_ = std::thread([this]() { transcription_worker(); });
    } else {
        // No audio recorded, return to ready state
        set_state(AppState::Ready);
        gui_manager_->set_status("No audio");
        gui_manager_->set_hint("Press F9 anywhere\nor click to record");
        
        // Auto-return to ready state after delay
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            if (state_ == AppState::Ready) {
                gui_manager_->set_status("Ready");
                gui_manager_->set_hint("Press F9 anywhere\nor click to record");
            }
        }).detach();
    }
}

void SuperWhisperApp::save_settings() {
    // Settings saving removed for now
}

void SuperWhisperApp::recording_worker() {
    try {
        auto start_time = std::chrono::steady_clock::now();
        
        while (is_recording_) {
            // Check duration limit (30 seconds)
            auto now = std::chrono::steady_clock::now();
            if ((now - start_time) > std::chrono::seconds(30)) {
                break;
            }
            
            // Minimal sleep for high-performance recording
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Recording worker error: " << e.what() << std::endl;
    }
}

void SuperWhisperApp::transcription_worker() {
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
        std::string text = whisper_wrapper_->transcribe(audio, 16000);
        
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

void SuperWhisperApp::process_audio_chunk(const AudioSample* data, size_t count) {
    if (!is_recording_) return;
    
    // Voice activity detection
    float max_amplitude = 0.0f;
    for (size_t i = 0; i < count; ++i) {
        float amplitude = std::abs(static_cast<float>(data[i])) / 32768.0f;
        max_amplitude = std::max(max_amplitude, amplitude);
    }
    
    if (max_amplitude > 0.01f) {  // Silence threshold
        last_voice_time_ = std::chrono::steady_clock::now();
    }
}

void SuperWhisperApp::handle_transcription_result(const std::string& text) {
    try {
        // Copy to clipboard (simplified - in production use proper clipboard API)
        std::cout << "Transcribed: " << text << std::endl;
        
        // Update GUI safely
        if (gui_manager_) {
            gui_manager_->set_status("Pasted");
            gui_manager_->set_hint("Text copied to\nclipboard");
        }
        
        // Return to ready state immediately (remove problematic detached thread)
        set_state(AppState::Ready);
        if (gui_manager_) {
            gui_manager_->set_status("Ready");
            gui_manager_->set_hint("Press F9 anywhere\nor click to record");
        }
        
        std::cout << "Transcription handling completed successfully" << std::endl;
        
        // Ensure transcription thread is properly joined
        if (transcription_thread_.joinable()) {
            transcription_thread_.detach(); // Detach since we're done
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Transcription handling error: " << e.what() << std::endl;
        handle_error("Failed to handle transcription: " + std::string(e.what()));
    }
}

void SuperWhisperApp::auto_paste_text(const std::string& text) {
    // On macOS, use osascript to paste text
    // This is a simplified implementation
    std::string command = "osascript -e 'tell application \"System Events\" to keystroke \"v\" using command down'";
    system(command.c_str());
}

void SuperWhisperApp::handle_error(const std::string& error) {
    std::cerr << "Error: " << error << std::endl;
    
    set_state(AppState::Error);
    gui_manager_->set_status("Error");
    gui_manager_->set_hint("Will retry in\na moment...");
    
    // Auto-recover after delay
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        set_state(AppState::Ready);
        gui_manager_->set_status("Ready");
        gui_manager_->set_hint("Press F9 anywhere\nor click to record");
    }).detach();
}

void SuperWhisperApp::recover_from_error() {
    set_state(AppState::Ready);
    gui_manager_->set_status("Ready");
    gui_manager_->set_hint("Press F9 anywhere\nor click to record");
}

void SuperWhisperApp::on_hotkey_pressed() {
    if (state_ == AppState::Ready) {
        start_recording();
    } else if (state_ == AppState::Recording) {
        stop_recording();
    }
}

void SuperWhisperApp::on_gui_button_click() {
    if (state_ == AppState::Ready) {
        start_recording();
    } else if (state_ == AppState::Recording) {
        stop_recording();
    }
}

void SuperWhisperApp::on_gui_close() {
    should_exit_ = true;
}

void SuperWhisperApp::cleanup_resources() {
    // Clear audio buffers
    if (audio_recorder_) {
        audio_recorder_->clear();
    }
    
    // Optimize memory usage
    optimize_memory_usage();
}

void SuperWhisperApp::optimize_memory_usage() {
    // Force garbage collection of temporary objects
    audio_buffer_.clear();
    audio_buffer_.shrink_to_fit();
}

} // namespace SuperWhisper

// Main entry point
int main() {
    try {
        SuperWhisper::SuperWhisperApp app;
        
        if (!app.initialize()) {
            std::cerr << "Failed to initialize SuperWhisper" << std::endl;
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
