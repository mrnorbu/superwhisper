#include "whisper_wrapper.hpp"
#include "whisper.h"
#include <iostream>
#include <algorithm>
#include <cmath>

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
    
    std::string transcribe(const AudioBuffer& audio, int sample_rate) override {
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
        
        // Configure transcription parameters for SPEED + STABILITY
        whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        
        // Optimize threading for Apple Silicon with GPU
        params.n_threads = std::min(4, (int)std::thread::hardware_concurrency()); // Conservative threading with GPU
        
        // Speed optimizations (more conservative)
        params.translate = false;
        params.language = "en";
        params.detect_language = false;
        params.suppress_blank = true;
        params.suppress_non_speech_tokens = true;
        params.token_timestamps = false;  // Disable timestamps for speed
        params.print_progress = false;    // Disable progress printing
        params.print_realtime = false;    // Disable real-time printing
        params.print_timestamps = false;  // Disable timestamp printing
        
        // Conservative thresholds for stability with GPU
        params.entropy_thold = 2.4f;      // Standard threshold
        params.logprob_thold = -1.0f;
        params.no_speech_thold = 0.6f;    // Standard silence detection
        
        // Run transcription
        int result = whisper_full(ctx_, params, resampled_audio.data(), resampled_audio.size());
        if (result != 0) {
            std::cerr << "Transcription failed with error: " << result << std::endl;
            return "";
        }
        
        // Extract text from segments
        std::string transcription;
        const int n_segments = whisper_full_n_segments(ctx_);
        
        for (int i = 0; i < n_segments; ++i) {
            const char* text = whisper_full_get_segment_text(ctx_, i);
            if (text) {
                transcription += text;
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
            size_t input_i = static_cast<size_t>(input_index);
            double fraction = input_index - input_i;
            
            if (input_i + 1 < input.size()) {
                output[i] = input[input_i] * (1.0 - fraction) + input[input_i + 1] * fraction;
            } else {
                output[i] = input[input_i];
            }
        }
        
        return output;
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
