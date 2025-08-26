#include "whisper_wrapper.hpp"
#include "settings.hpp"
#include "whisper.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <thread>

namespace SuperWhisper {

class WhisperCppWrapper : public WhisperWrapper {
public:
    WhisperCppWrapper() : ctx_(nullptr), is_loaded_(false) {}
    
    ~WhisperCppWrapper() override {
        unload_model();
    }
    
    bool load_model(const std::string& path) override {
        if (is_loaded_) {
            unload_model();
        }
        
        // Load model with Apple Silicon optimizations
        // Use the new API for this whisper.cpp version
        struct whisper_context_params cparams = whisper_context_default_params();
        
        // CRITICAL: Enable Metal GPU acceleration on Apple Silicon for massive speed boost
        cparams.use_gpu = true;  // Enable Metal/GPU acceleration
        
        ctx_ = whisper_init_from_file_with_params(path.c_str(), cparams);
        if (!ctx_) {
            std::cerr << "Failed to load Whisper model: " << path << std::endl;
            return false;
        }
        
        is_loaded_ = true;
        model_path_ = path;
        
        std::cout << "Whisper model loaded successfully: " << path << std::endl;
        std::cout << "Memory usage: " << get_memory_usage() / (1024 * 1024) << " MB" << std::endl;
        
        return true;
    }
    
    std::string transcribe(const AudioBuffer& audio, int sample_rate, const Settings& settings) override {
        if (!is_loaded_ || !ctx_) {
            return "";
        }
        
        if (audio.empty()) {
            return "";
        }
        
        // Prepare audio data for Whisper with memory optimization
        // Whisper expects 16kHz, 16-bit PCM, mono
        std::vector<float> audio_float;
        audio_float.reserve(audio.size() + 1024);  // Extra space to avoid reallocations
        
        // Convert int16 to float32 and normalize to [-1, 1]
        for (int16_t sample : audio) {
            audio_float.push_back(static_cast<float>(sample) / 32768.0f);
        }
        
        // Resample if necessary with memory optimization
        std::vector<float> resampled_audio;
        if (sample_rate != 16000) {
            resampled_audio = resample_audio(audio_float, sample_rate, 16000);
        } else {
            resampled_audio = std::move(audio_float);
        }
        
        // Shrink to fit to free unused memory before inference
        resampled_audio.shrink_to_fit();
        
        // Configure transcription parameters from settings
        whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        
        // Use settings for all configurable parameters
        params.n_threads = settings.num_threads;
        params.translate = settings.translate_to_english;
        params.language = settings.language == "auto" ? nullptr : settings.language.c_str();
        params.detect_language = (settings.language == "auto");
        params.max_tokens = settings.max_tokens;
        params.temperature = settings.temperature;
        
        // Note: top_p, top_k, and repetition_penalty are not available in whisper.cpp
        // These are kept in settings for future compatibility but not used here
        
        // Output formatting settings
        params.suppress_blank = settings.suppress_blank;
        params.suppress_non_speech_tokens = settings.suppress_non_speech_tokens;
        params.token_timestamps = settings.print_timestamps;
        params.print_progress = settings.print_progress;
        params.print_realtime = false;  // Always false for CLI
        params.print_timestamps = settings.print_timestamps;
        
        // Note: print_colors and print_special are not available in whisper.cpp
        // These are kept in settings for future compatibility but not used here
        
        // Thresholds (now configurable from settings)
        params.entropy_thold = settings.entropy_threshold;
        params.logprob_thold = settings.logprob_threshold;
        params.no_speech_thold = settings.no_speech_threshold;
        
        // Run transcription
        int result = whisper_full(ctx_, params, resampled_audio.data(), resampled_audio.size());
        if (result != 0) {
            std::cerr << "Transcription failed with error: " << result << std::endl;
            return "";
        }
        
        // Extract text based on output format
        std::string transcription;
        
        if (settings.output_format == "json") {
            // JSON output format
            transcription = "{\n  \"segments\": [\n";
            const int n_segments = whisper_full_n_segments(ctx_);
            
            for (int i = 0; i < n_segments; ++i) {
                const char* text = whisper_full_get_segment_text(ctx_, i);
                float start_time = whisper_full_get_segment_t0(ctx_, i) / 100.0f;
                float end_time = whisper_full_get_segment_t1(ctx_, i) / 100.0f;
                
                if (text) {
                    if (i > 0) transcription += ",\n";
                    transcription += "    {\n";
                    transcription += "      \"id\": " + std::to_string(i) + ",\n";
                    transcription += "      \"start\": " + std::to_string(start_time) + ",\n";
                    transcription += "      \"end\": " + std::to_string(end_time) + ",\n";
                    transcription += "      \"text\": \"" + std::string(text) + "\"\n";
                    transcription += "    }";
                }
            }
            transcription += "\n  ]\n}";
            
        } else if (settings.output_format == "srt") {
            // SRT subtitle format
            const int n_segments = whisper_full_n_segments(ctx_);
            
            for (int i = 0; i < n_segments; ++i) {
                const char* text = whisper_full_get_segment_text(ctx_, i);
                float start_time = whisper_full_get_segment_t0(ctx_, i) / 100.0f;
                float end_time = whisper_full_get_segment_t1(ctx_, i) / 100.0f;
                
                if (text) {
                    transcription += std::to_string(i + 1) + "\n";
                    transcription += format_time_srt(start_time) + " --> " + format_time_srt(end_time) + "\n";
                    transcription += std::string(text) + "\n\n";
                }
            }
            
        } else if (settings.output_format == "vtt") {
            // VTT subtitle format
            transcription = "WEBVTT\n\n";
            const int n_segments = whisper_full_n_segments(ctx_);
            
            for (int i = 0; i < n_segments; ++i) {
                const char* text = whisper_full_get_segment_text(ctx_, i);
                float start_time = whisper_full_get_segment_t0(ctx_, i) / 100.0f;
                float end_time = whisper_full_get_segment_t1(ctx_, i) / 100.0f;
                
                if (text) {
                    transcription += format_time_vtt(start_time) + " --> " + format_time_vtt(end_time) + "\n";
                    transcription += std::string(text) + "\n\n";
                }
            }
            
        } else if (settings.output_format == "csv") {
            // CSV format
            transcription = "start_time,end_time,text\n";
            const int n_segments = whisper_full_n_segments(ctx_);
            
            for (int i = 0; i < n_segments; ++i) {
                const char* text = whisper_full_get_segment_text(ctx_, i);
                float start_time = whisper_full_get_segment_t0(ctx_, i) / 100.0f;
                float end_time = whisper_full_get_segment_t1(ctx_, i) / 100.0f;
                
                if (text) {
                    transcription += std::to_string(start_time) + "," + std::to_string(end_time) + ",\"" + std::string(text) + "\"\n";
                }
            }
            
        } else {
            // Default text format
            const int n_segments = whisper_full_n_segments(ctx_);
            
            for (int i = 0; i < n_segments; ++i) {
                const char* text = whisper_full_get_segment_text(ctx_, i);
                if (text) {
                    transcription += text;
                }
            }
        }
        
        return transcription;
    }
    
    bool is_loaded() const override {
        return is_loaded_ && ctx_ != nullptr;
    }
    
    void unload_model() override {
        if (ctx_) {
            whisper_free(ctx_);
            ctx_ = nullptr;
        }
        is_loaded_ = false;
        model_path_.clear();
    }
    
    size_t get_memory_usage() const override {
        if (!ctx_) return 0;
        
        // Estimate memory usage based on model size
        // This is approximate - whisper.cpp doesn't expose exact memory usage
        size_t estimated_size = 0;
        
        // Base model sizes (approximate)
        if (model_path_.find("tiny") != std::string::npos) {
            estimated_size = 39 * 1024 * 1024;  // ~39 MB
        } else if (model_path_.find("base") != std::string::npos) {
            estimated_size = 74 * 1024 * 1024;  // ~74 MB
        } else if (model_path_.find("small") != std::string::npos) {
            estimated_size = 244 * 1024 * 1024;  // ~244 MB
        } else if (model_path_.find("medium") != std::string::npos) {
            estimated_size = 769 * 1024 * 1024;  // ~769 MB
        } else if (model_path_.find("large") != std::string::npos) {
            estimated_size = 1550 * 1024 * 1024;  // ~1.55 GB
        } else {
            estimated_size = 100 * 1024 * 1024;  // Default estimate
        }
        
        return estimated_size;
    }
    
private:
    // Simple audio resampling (linear interpolation)
    // For production, use a proper resampling library
    std::vector<float> resample_audio(const std::vector<float>& input, int input_rate, int output_rate) {
        if (input_rate == output_rate) {
            return input;
        }
        
        double ratio = static_cast<double>(output_rate) / input_rate;
        size_t output_size = static_cast<size_t>(input.size() * ratio);
        std::vector<float> output(output_size);
        
        for (size_t i = 0; i < output_size; ++i) {
            double input_index = i / ratio;
            size_t index1 = static_cast<size_t>(input_index);
            size_t index2 = std::min(index1 + 1, input.size() - 1);
            double fraction = input_index - index1;
            
            output[i] = input[index1] * (1.0 - fraction) + input[index2] * fraction;
        }
        
        return output;
    }
    
    // Helper function to format time for SRT format (HH:MM:SS,mmm)
    std::string format_time_srt(float seconds) {
        int hours = static_cast<int>(seconds) / 3600;
        int minutes = (static_cast<int>(seconds) % 3600) / 60;
        int secs = static_cast<int>(seconds) % 60;
        int millisecs = static_cast<int>((seconds - static_cast<int>(seconds)) * 1000);
        
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d,%03d", hours, minutes, secs, millisecs);
        return std::string(buffer);
    }
    
    // Helper function to format time for VTT format (HH:MM:SS.mmm)
    std::string format_time_vtt(float seconds) {
        int hours = static_cast<int>(seconds) / 3600;
        int minutes = (static_cast<int>(seconds) % 3600) / 60;
        int secs = static_cast<int>(seconds) % 60;
        int millisecs = static_cast<int>((seconds - static_cast<int>(seconds)) * 1000);
        
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", hours, minutes, secs, millisecs);
        return std::string(buffer);
    }
    
    whisper_context* ctx_;
    bool is_loaded_;
    std::string model_path_;
};

// Factory function
std::unique_ptr<WhisperWrapper> create_whisper_wrapper() {
    return std::make_unique<WhisperCppWrapper>();
}

} // namespace SuperWhisper
