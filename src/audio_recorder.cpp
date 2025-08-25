#include "audio_recorder.hpp"
#include <portaudio.h>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace SuperWhisper {

class PortAudioRecorder : public AudioRecorder {
public:
    PortAudioRecorder() : stream_(nullptr), is_recording_(false), callback_(nullptr) {
        // Initialize PortAudio
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            throw std::runtime_error("Failed to initialize PortAudio: " + std::string(Pa_GetErrorText(err)));
        }
    }
    
    ~PortAudioRecorder() override {
        stop();
        Pa_Terminate();
    }
    
    bool start() override {
        if (is_recording_) return false;
        
        // Configure stream parameters for optimal performance
        PaStreamParameters input_params;
        input_params.device = Pa_GetDefaultInputDevice();
        if (input_params.device == paNoDevice) {
            std::cerr << "No input device found" << std::endl;
            return false;
        }
        
        input_params.channelCount = 1;  // Mono for efficiency
        input_params.sampleFormat = paInt16;  // 16-bit for memory efficiency
        input_params.suggestedLatency = Pa_GetDeviceInfo(input_params.device)->defaultLowInputLatency;
        input_params.hostApiSpecificStreamInfo = nullptr;
        
        // Open stream with optimal buffer size for Apple Silicon
        PaError err = Pa_OpenStream(
            &stream_,
            &input_params,
            nullptr,  // No output
            16000,    // Sample rate
            512,      // Frames per buffer - optimized for Apple Silicon
            paClipOff | paDitherOff,  // Disable unnecessary processing
            &PortAudioRecorder::pa_callback,
            this
        );
        
        if (err != paNoError) {
            std::cerr << "Failed to open stream: " << Pa_GetErrorText(err) << std::endl;
            return false;
        }
        
        // Start stream
        err = Pa_StartStream(stream_);
        if (err != paNoError) {
            std::cerr << "Failed to start stream: " << Pa_GetErrorText(err) << std::endl;
            Pa_CloseStream(stream_);
            stream_ = nullptr;
            return false;
        }
        
        is_recording_ = true;
        return true;
    }
    
    void stop() override {
        if (!is_recording_) return;
        
        is_recording_ = false;
        
        if (stream_) {
            Pa_StopStream(stream_);
            Pa_CloseStream(stream_);
            stream_ = nullptr;
        }
    }
    
    bool is_recording() const override {
        return is_recording_;
    }
    
    AudioBuffer get_audio() const override {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        return audio_buffer_;
    }
    
    void clear() override {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        audio_buffer_.clear();
        audio_buffer_.shrink_to_fit();  // Free memory
    }
    
    void set_audio_callback(std::function<void(const AudioSample*, size_t)> callback) override {
        callback_ = callback;
    }
    
private:
    // PortAudio callback - highly optimized for Apple Silicon
    static int pa_callback(const void* input, void* output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData) {
        auto* recorder = static_cast<PortAudioRecorder*>(userData);
        
        if (statusFlags & paInputOverflow) {
            // Handle overflow gracefully
            return paContinue;
        }
        
        if (input && recorder->callback_) {
            // Process audio data efficiently
            const AudioSample* samples = static_cast<const AudioSample*>(input);
            
            // Use callback for real-time processing
            recorder->callback_(samples, frameCount);
            
            // Also store in buffer for transcription
            recorder->add_audio_chunk(samples, frameCount);
        }
        
        return paContinue;
    }
    
    void add_audio_chunk(const AudioSample* samples, size_t count) {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        
        // Memory-efficient buffer management
        const size_t max_buffer_size = 16000 * 30;  // 30 seconds max
        
        if (audio_buffer_.size() + count > max_buffer_size) {
            // Remove oldest samples (sliding window)
            size_t remove_count = audio_buffer_.size() + count - max_buffer_size;
            audio_buffer_.erase(audio_buffer_.begin(), audio_buffer_.begin() + remove_count);
        }
        
        // Add new samples
        audio_buffer_.insert(audio_buffer_.end(), samples, samples + count);
    }
    
    PaStream* stream_;
    std::atomic<bool> is_recording_;
    std::function<void(const AudioSample*, size_t)> callback_;
    
    // Thread-safe audio buffer
    mutable std::mutex buffer_mutex_;
    AudioBuffer audio_buffer_;
};

// Factory function
std::unique_ptr<AudioRecorder> create_audio_recorder() {
    return std::make_unique<PortAudioRecorder>();
}

} // namespace SuperWhisper
