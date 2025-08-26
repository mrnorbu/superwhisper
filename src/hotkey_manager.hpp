#pragma once

#include <functional>
#include <string>
#include <memory>

namespace SuperWhisper {

// Hotkey manager interface for CLI version
class HotkeyManager {
public:
    virtual ~HotkeyManager() = default;
    
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    
    // Register hotkeys
    virtual bool register_start_hotkey(const std::string& key, std::function<void()> callback) = 0;
    virtual bool register_stop_hotkey(const std::string& key, std::function<void()> callback) = 0;
    virtual bool register_quit_hotkey(const std::string& key, std::function<void()> callback) = 0;
    
    // Unregister all hotkeys
    virtual void unregister_all_hotkeys() = 0;
    
    // Check if hotkeys are supported
    virtual bool is_supported() const = 0;
};

// Factory function for creating hotkey manager
std::unique_ptr<HotkeyManager> create_hotkey_manager();

} // namespace SuperWhisper
