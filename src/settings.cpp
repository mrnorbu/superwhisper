#include "settings.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace SuperWhisper {

void Settings::save(const std::string& path) {
    try {
        // Expand tilde to home directory
        std::string expanded_path = path;
        if (expanded_path[0] == '~') {
            const char* home = getenv("HOME");
            if (home) {
                expanded_path = std::string(home) + expanded_path.substr(1);
            }
        }
        
        // Create directory if it doesn't exist
        std::filesystem::path config_path(expanded_path);
        std::filesystem::create_directories(config_path.parent_path());
        
        // Create JSON object
        nlohmann::json j;
        
        // Model settings
        j["model_path"] = model_path;
        j["model_size"] = model_size;
        
        // Audio settings
        j["silence_duration"] = silence_duration;
        j["max_duration"] = max_duration;
        j["silence_threshold"] = silence_threshold;
        j["sample_rate"] = sample_rate;
        
        // Whisper settings
        j["language"] = language;
        j["translate_to_english"] = translate_to_english;
        j["num_threads"] = num_threads;
        j["max_tokens"] = max_tokens;
        j["temperature"] = temperature;
        j["top_p"] = top_p;
        j["top_k"] = top_k;
        j["repetition_penalty"] = repetition_penalty;
        j["print_timestamps"] = print_timestamps;
        j["print_colors"] = print_colors;
        j["print_special"] = print_special;
        j["print_progress"] = print_progress;
        j["print_tokens"] = print_tokens;
        
        // Additional Whisper settings
        j["entropy_threshold"] = entropy_threshold;
        j["logprob_threshold"] = logprob_threshold;
        j["no_speech_threshold"] = no_speech_threshold;
        j["suppress_blank"] = suppress_blank;
        j["suppress_non_speech_tokens"] = suppress_non_speech_tokens;
        
        // Output settings
        j["output_format"] = output_format;
        j["output_file"] = output_file;
        j["copy_to_clipboard"] = copy_to_clipboard;
        
        // Performance settings
        j["use_gpu"] = use_gpu;
        j["use_metal"] = use_metal;
        j["use_accelerate"] = use_accelerate;
        
        // Hotkey settings
        j["enable_hotkeys"] = enable_hotkeys;
        j["start_hotkey"] = start_hotkey;
        j["stop_hotkey"] = stop_hotkey;
        j["quit_hotkey"] = quit_hotkey;
        
        // Input mode settings
        j["input_mode"] = input_mode;
        j["enable_terminal_input"] = enable_terminal_input;
        j["enable_global_hotkeys"] = enable_global_hotkeys;
        
        // Save to file
        std::ofstream file(expanded_path);
        if (file.is_open()) {
            file << j.dump(2);
            file.close();
            std::cout << "Settings saved to: " << expanded_path << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to save settings: " << e.what() << std::endl;
    }
}

void Settings::load(const std::string& path) {
    try {
        // Expand tilde to home directory
        std::string expanded_path = path;
        if (expanded_path[0] == '~') {
            const char* home = getenv("HOME");
            if (home) {
                expanded_path = std::string(home) + expanded_path.substr(1);
            }
        }
        
        // Check if file exists
        if (!std::filesystem::exists(expanded_path)) {
            std::cout << "Config file not found, using defaults: " << expanded_path << std::endl;
            return; // Use defaults
        }
        
        // Read and parse JSON
        std::ifstream file(expanded_path);
        if (file.is_open()) {
            nlohmann::json j;
            file >> j;
            file.close();
            
            // Load model settings
            if (j.contains("model_path")) model_path = j["model_path"];
            if (j.contains("model_size")) model_size = j["model_size"];
            
            // Load audio settings
            if (j.contains("silence_duration")) silence_duration = j["silence_duration"];
            if (j.contains("max_duration")) max_duration = j["max_duration"];
            if (j.contains("silence_threshold")) silence_threshold = j["silence_threshold"];
            if (j.contains("sample_rate")) sample_rate = j["sample_rate"];
            
            // Load Whisper settings
            if (j.contains("language")) language = j["language"];
            if (j.contains("translate_to_english")) translate_to_english = j["translate_to_english"];
            if (j.contains("num_threads")) num_threads = j["num_threads"];
            if (j.contains("max_tokens")) max_tokens = j["max_tokens"];
            if (j.contains("temperature")) temperature = j["temperature"];
            if (j.contains("top_p")) top_p = j["top_p"];
            if (j.contains("top_k")) top_k = j["top_k"];
            if (j.contains("repetition_penalty")) repetition_penalty = j["repetition_penalty"];
            if (j.contains("print_timestamps")) print_timestamps = j["print_timestamps"];
            if (j.contains("print_colors")) print_colors = j["print_colors"];
            if (j.contains("print_special")) print_special = j["print_special"];
            if (j.contains("print_progress")) print_progress = j["print_progress"];
            if (j.contains("print_tokens")) print_tokens = j["print_tokens"];
            
            // Load additional Whisper settings
            if (j.contains("entropy_threshold")) entropy_threshold = j["entropy_threshold"];
            if (j.contains("logprob_threshold")) logprob_threshold = j["logprob_threshold"];
            if (j.contains("no_speech_threshold")) no_speech_threshold = j["no_speech_threshold"];
            if (j.contains("suppress_blank")) suppress_blank = j["suppress_blank"];
            if (j.contains("suppress_non_speech_tokens")) suppress_non_speech_tokens = j["suppress_non_speech_tokens"];
            
            // Load output settings
            if (j.contains("output_format")) output_format = j["output_format"];
            if (j.contains("output_file")) output_file = j["output_file"];
            if (j.contains("copy_to_clipboard")) copy_to_clipboard = j["copy_to_clipboard"];
            
            // Load performance settings
            if (j.contains("use_gpu")) use_gpu = j["use_gpu"];
            if (j.contains("use_metal")) use_metal = j["use_metal"];
            if (j.contains("use_accelerate")) use_accelerate = j["use_accelerate"];
            
            // Load hotkey settings
            if (j.contains("enable_hotkeys")) enable_hotkeys = j["enable_hotkeys"];
            if (j.contains("start_hotkey")) start_hotkey = j["start_hotkey"];
            if (j.contains("stop_hotkey")) stop_hotkey = j["stop_hotkey"];
            if (j.contains("quit_hotkey")) quit_hotkey = j["quit_hotkey"];
            
            // Load input mode settings
            if (j.contains("input_mode")) input_mode = j["input_mode"];
            if (j.contains("enable_terminal_input")) enable_terminal_input = j["enable_terminal_input"];
            if (j.contains("enable_global_hotkeys")) enable_global_hotkeys = j["enable_global_hotkeys"];
            
            std::cout << "Settings loaded from: " << expanded_path << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load settings: " << e.what() << std::endl;
        // Keep defaults on error
    }
}

void Settings::print_help() const {
    std::cout << "SuperWhisper CLI - Available Settings:\n\n";
    
    std::cout << "Model Settings:\n";
    std::cout << "  model_path: Path to Whisper model file\n";
    std::cout << "  model_size: Model size (tiny, base, small, medium, large)\n\n";
    
    std::cout << "Audio Settings:\n";
    std::cout << "  silence_duration: Duration of silence to stop recording (seconds)\n";
    std::cout << "  max_duration: Maximum recording duration (seconds)\n";
    std::cout << "  silence_threshold: Audio threshold for silence detection\n";
    std::cout << "  sample_rate: Audio sample rate (Hz)\n\n";
    
    std::cout << "Whisper Settings:\n";
    std::cout << "  language: Language code or 'auto' for detection\n";
    std::cout << "  translate_to_english: Translate output to English\n";
    std::cout << "  num_threads: Number of CPU threads to use\n";
    std::cout << "  max_tokens: Maximum tokens in output\n";
    std::cout << "  temperature: Sampling temperature (0.0 = deterministic)\n";
    std::cout << "  top_p: Nucleus sampling parameter\n";
    std::cout << "  top_k: Top-k sampling parameter\n";
    std::cout << "  repetition_penalty: Penalty for repetition\n";
    std::cout << "  print_timestamps: Include timestamps in output\n";
    std::cout << "  print_colors: Use colored output\n";
    std::cout << "  print_special: Include special tokens\n";
    std::cout << "  print_progress: Show transcription progress\n";
    std::cout << "  print_tokens: Show individual tokens\n";
    std::cout << "  entropy_threshold: Threshold for entropy\n";
    std::cout << "  logprob_threshold: Threshold for log probability\n";
    std::cout << "  no_speech_threshold: Threshold for no speech\n";
    std::cout << "  suppress_blank: Suppress blank tokens\n";
    std::cout << "  suppress_non_speech_tokens: Suppress non-speech tokens\n\n";
    
    std::cout << "Output Settings:\n";
    std::cout << "  output_format: Output format (text, json, srt, vtt, csv)\n";
    std::cout << "  output_file: Output file path (empty for stdout)\n";
    std::cout << "  copy_to_clipboard: Copy result to clipboard\n\n";
    
    std::cout << "Performance Settings:\n";
    std::cout << "  use_gpu: Enable GPU acceleration\n";
    std::cout << "  use_metal: Enable Metal GPU on macOS\n";
    std::cout << "  use_accelerate: Enable Accelerate framework\n\n";
    
    std::cout << "Hotkey Settings:\n";
    std::cout << "  enable_hotkeys: Enable global hotkey support\n";
    std::cout << "  start_hotkey: Key to start recording (e.g., F9)\n";
    std::cout << "  stop_hotkey: Key to stop recording (e.g., F10)\n";
    std::cout << "  quit_hotkey: Key to quit application (e.g., F12)\n\n";
}

void Settings::print_current_settings() const {
    std::cout << "Current Settings:\n";
    std::cout << "================\n";
    
    std::cout << "Model: " << model_path << " (" << model_size << ")\n";
    std::cout << "Audio: " << sample_rate << "Hz, " << max_duration << "s max, " 
              << silence_threshold << " threshold\n";
    std::cout << "Language: " << language << (translate_to_english ? " → English" : "") << "\n";
    std::cout << "Threads: " << num_threads << ", Temperature: " << temperature << "\n";
    std::cout << "Top-p: " << top_p << ", Top-k: " << top_k << ", Repetition Penalty: " << repetition_penalty << "\n";
    std::cout << "Thresholds: Entropy=" << entropy_threshold << ", LogProb=" << logprob_threshold << ", NoSpeech=" << no_speech_threshold << "\n";
    std::cout << "Output: " << output_format << (output_file.empty() ? " (stdout)" : " → " + output_file) << "\n";
    std::cout << "GPU: " << (use_gpu ? "Yes" : "No") << ", Metal: " << (use_metal ? "Yes" : "No") << "\n";
    std::cout << "Hotkeys: " << (enable_hotkeys ? "Yes" : "No");
    if (enable_hotkeys) {
        std::cout << " (Start: " << start_hotkey << ", Stop: " << stop_hotkey << ", Quit: " << quit_hotkey << ")";
    }
    std::cout << "\n";
}

} // namespace SuperWhisper
