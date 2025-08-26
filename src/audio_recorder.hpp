#pragma once

#include <memory>
#include <vector>
#include <functional>

namespace SuperWhisper {

// Audio sample type - using 16-bit for memory efficiency
using AudioSample = int16_t;
using AudioBuffer = std::vector<AudioSample>;

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

// Factory function for creating audio recorder
std::unique_ptr<AudioRecorder> create_audio_recorder();

} // namespace SuperWhisper
