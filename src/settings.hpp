#pragma once

#include <string>

namespace SuperWhisper {

// Settings structure for CLI version
struct Settings {
    // Model settings
    std::string model_path = "model/ggml-base.en-q5_1.bin";
    std::string model_size = "base";
    
    // Audio settings
    float silence_duration = 1.0f;
    int max_duration = 30;
    float silence_threshold = 0.01f;
    int sample_rate = 16000;
    
    // Whisper settings
    std::string language = "auto";
    bool translate_to_english = false;
    int num_threads = 4;
    int max_tokens = 448;
    float temperature = 0.0f;
    float top_p = 1.0f;
    float top_k = 40;
    float repetition_penalty = 1.1f;
    bool print_timestamps = false;
    bool print_colors = false;
    bool print_special = false;
    bool print_progress = true;
    bool print_tokens = false;
    
    // Additional Whisper settings
    float entropy_threshold = 2.4f;
    float logprob_threshold = -1.0f;
    float no_speech_threshold = 0.6f;
    bool suppress_blank = true;
    bool suppress_non_speech_tokens = true;
    
    // Output settings
    std::string output_format = "text"; // text, json, srt, vtt, csv
    std::string output_file = "";
    bool copy_to_clipboard = true;
    
    // Performance settings
    bool use_gpu = true;
    bool use_metal = true;
    bool use_accelerate = true;
    
    // Hotkey settings
    bool enable_hotkeys;
    std::string start_hotkey;
    std::string stop_hotkey;
    std::string quit_hotkey;
    
    // Input mode settings
    std::string input_mode; // "terminal", "global", "both"
    bool enable_terminal_input;  // Enable r, s, q keys in terminal
    bool enable_global_hotkeys;  // Enable F9, F10, F12 global hotkeys
    
    void save(const std::string& path);
    void load(const std::string& path);
    void print_help() const;
    void print_current_settings() const;
};

} // namespace SuperWhisper
