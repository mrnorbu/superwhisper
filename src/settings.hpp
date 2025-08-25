#pragma once

#include <string>

namespace SuperWhisper {

// Settings structure - packed for memory efficiency
struct Settings {
    std::string model_path = "model/ggml-base.en-q5_1.bin";
    float silence_duration = 1.0f;
    int max_duration = 30;
    float silence_threshold = 0.01f;
    int sample_rate = 16000;
    bool auto_paste = true;
    int window_x = 1200;
    int window_y = 120;
    
    // Memory optimization: use smaller model by default
    std::string model_size = "base";
    
    void save(const std::string& path);
    void load(const std::string& path);
};

} // namespace SuperWhisper
