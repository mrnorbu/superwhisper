#include "settings.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>

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
        
        // Save settings to JSON
        std::ofstream file(expanded_path);
        if (file.is_open()) {
            file << "{\n";
            file << "  \"model_path\": \"" << model_path << "\",\n";
            file << "  \"silence_duration\": " << silence_duration << ",\n";
            file << "  \"max_duration\": " << max_duration << ",\n";
            file << "  \"silence_threshold\": " << silence_threshold << ",\n";
            file << "  \"sample_rate\": " << sample_rate << ",\n";
            file << "  \"auto_paste\": " << (auto_paste ? "true" : "false") << ",\n";
            file << "  \"window_x\": " << window_x << ",\n";
            file << "  \"window_y\": " << window_y << ",\n";
            file << "  \"model_size\": \"" << model_size << "\"\n";
            file << "}\n";
            file.close();
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
            return; // Use defaults
        }
        
        // Simple JSON parsing (for production, use a proper JSON library)
        std::ifstream file(expanded_path);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                // Parse key-value pairs
                if (line.find("\"model_path\"") != std::string::npos) {
                    size_t start = line.find("\"", line.find(":") + 1) + 1;
                    size_t end = line.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        model_path = line.substr(start, end - start);
                    }
                } else if (line.find("\"silence_duration\"") != std::string::npos) {
                    size_t pos = line.find(":") + 1;
                    silence_duration = std::stof(line.substr(pos));
                } else if (line.find("\"max_duration\"") != std::string::npos) {
                    size_t pos = line.find(":") + 1;
                    max_duration = std::stoi(line.substr(pos));
                } else if (line.find("\"silence_threshold\"") != std::string::npos) {
                    size_t pos = line.find(":") + 1;
                    silence_threshold = std::stof(line.substr(pos));
                } else if (line.find("\"sample_rate\"") != std::string::npos) {
                    size_t pos = line.find(":") + 1;
                    sample_rate = std::stoi(line.substr(pos));
                } else if (line.find("\"auto_paste\"") != std::string::npos) {
                    size_t pos = line.find(":") + 1;
                    auto_paste = (line.find("true", pos) != std::string::npos);
                } else if (line.find("\"window_x\"") != std::string::npos) {
                    size_t pos = line.find(":") + 1;
                    window_x = std::stoi(line.substr(pos));
                } else if (line.find("\"window_y\"") != std::string::npos) {
                    size_t pos = line.find(":") + 1;
                    window_y = std::stoi(line.substr(pos));
                } else if (line.find("\"model_size\"") != std::string::npos) {
                    size_t start = line.find("\"", line.find(":") + 1) + 1;
                    size_t end = line.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        model_size = line.substr(start, end - start);
                    }
                }
            }
            file.close();
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to load settings: " << e.what() << std::endl;
        // Keep defaults on error
    }
}

} // namespace SuperWhisper
