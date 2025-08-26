#pragma once

#include <memory>
#include <string>
#include <vector>

namespace SuperWhisper {

// Forward declaration
struct Settings;

// Audio sample type - using 16-bit for memory efficiency
using AudioSample = int16_t;
using AudioBuffer = std::vector<AudioSample>;

// Whisper wrapper interface
class WhisperWrapper {
public:
    virtual ~WhisperWrapper() = default;
    
    virtual bool load_model(const std::string& path) = 0;
    virtual std::string transcribe(const AudioBuffer& audio, int sample_rate, const Settings& settings) = 0;
    virtual bool is_loaded() const = 0;
    
    // Memory management
    virtual void unload_model() = 0;
    virtual size_t get_memory_usage() const = 0;
};

// Factory function for creating Whisper wrapper
std::unique_ptr<WhisperWrapper> create_whisper_wrapper();

} // namespace SuperWhisper
