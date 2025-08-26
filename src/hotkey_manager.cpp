#include "hotkey_manager.hpp"
#include <iostream>
#include <map>
#include <thread>
#include <atomic>

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace SuperWhisper {

#ifdef __APPLE__

// macOS-specific hotkey manager using Carbon framework
class CarbonHotkeyManager : public HotkeyManager {
public:
    CarbonHotkeyManager() : is_initialized_(false) {}
    
    ~CarbonHotkeyManager() override {
        shutdown();
    }
    
    bool initialize() override {
        if (is_initialized_) return true;
        
        // Check if accessibility permissions are granted
        if (!check_accessibility_permissions()) {
            std::cout << "Warning: Accessibility permissions not granted. Hotkeys may not work." << std::endl;
            std::cout << "To enable hotkeys, go to System Preferences > Security & Privacy > Accessibility" << std::endl;
            std::cout << "and add Terminal (or your terminal app) to the list." << std::endl;
        }
        
        is_initialized_ = true;
        return true;
    }
    
    void shutdown() override {
        if (!is_initialized_) return;
        
        unregister_all_hotkeys();
        is_initialized_ = false;
    }
    
    bool register_start_hotkey(const std::string& key, std::function<void()> callback) override {
        if (!is_initialized_) return false;
        
        start_callback_ = callback;
        return register_carbon_hotkey(key, start_hotkey_id_);
    }
    
    bool register_stop_hotkey(const std::string& key, std::function<void()> callback) override {
        if (!is_initialized_) return false;
        
        stop_callback_ = callback;
        return register_carbon_hotkey(key, stop_hotkey_id_);
    }
    
    bool register_quit_hotkey(const std::string& key, std::function<void()> callback) override {
        if (!is_initialized_) return false;
        
        quit_callback_ = callback;
        return register_carbon_hotkey(key, quit_hotkey_id_);
    }
    
    void unregister_all_hotkeys() override {
        if (start_hotkey_id_ != 0) {
            UnregisterEventHotKey(start_hotkey_id_);
            start_hotkey_id_ = 0;
        }
        if (stop_hotkey_id_ != 0) {
            UnregisterEventHotKey(stop_hotkey_id_);
            stop_hotkey_id_ = 0;
        }
        if (quit_hotkey_id_ != 0) {
            UnregisterEventHotKey(quit_hotkey_id_);
            quit_hotkey_id_ = 0;
        }
    }
    
    bool is_supported() const override {
        return true;
    }
    
private:
    bool check_accessibility_permissions() {
        // Check if we have accessibility permissions
        return AXIsProcessTrusted();
    }
    
    bool register_carbon_hotkey(const std::string& key, EventHotKeyRef& hotkey_ref) {
        // Parse key string (e.g., "F9", "F10", "F12")
        UInt32 key_code = 0;
        UInt32 modifiers = 0;
        
        if (key == "F9") key_code = kVK_F9;
        else if (key == "F10") key_code = kVK_F10;
        else if (key == "F12") key_code = kVK_F12;
        else {
            std::cerr << "Unsupported hotkey: " << key << std::endl;
            return false;
        }
        
        // Create event target
        EventTargetRef target = GetApplicationEventTarget();
        if (!target) {
            std::cerr << "Failed to get application event target" << std::endl;
            return false;
        }
        
        // Install event handler
        EventTypeSpec event_type;
        event_type.eventClass = kEventClassKeyboard;
        event_type.eventKind = kEventHotKeyPressed;
        
        OSStatus status = InstallEventHandler(target, hotkey_handler, 1, &event_type, this, nullptr);
        if (status != noErr) {
            std::cerr << "Failed to install event handler: " << status << std::endl;
            return false;
        }
        
        // Register hotkey
        EventHotKeyID hotkey_id;
        hotkey_id.signature = 'htk1';
        hotkey_id.id = 1;
        
        status = RegisterEventHotKey(key_code, modifiers, hotkey_id, target, 0, &hotkey_ref);
        if (status != noErr) {
            std::cerr << "Failed to register hotkey: " << status << std::endl;
            return false;
        }
        
        std::cout << "Registered hotkey: " << key << std::endl;
        return true;
    }
    
    static OSStatus hotkey_handler(EventHandlerCallRef next_handler, EventRef event, void* user_data) {
        CarbonHotkeyManager* manager = static_cast<CarbonHotkeyManager*>(user_data);
        
        EventHotKeyID hotkey_id;
        GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, nullptr, sizeof(hotkey_id), nullptr, &hotkey_id);
        
        // Determine which hotkey was pressed and call appropriate callback
        if (hotkey_id.id == 1) {
            if (manager->start_callback_) manager->start_callback_();
        }
        // Add more hotkey handling as needed
        
        return noErr;
    }
    
    bool is_initialized_;
    EventHotKeyRef start_hotkey_id_ = 0;
    EventHotKeyRef stop_hotkey_id_ = 0;
    EventHotKeyRef quit_hotkey_id_ = 0;
    
    std::function<void()> start_callback_;
    std::function<void()> stop_callback_;
    std::function<void()> quit_callback_;
};

#else

// Fallback hotkey manager for non-macOS systems
class FallbackHotkeyManager : public HotkeyManager {
public:
    bool initialize() override {
        std::cout << "Warning: Hotkeys not supported on this platform" << std::endl;
        return false;
    }
    
    void shutdown() override {}
    
    bool register_start_hotkey(const std::string& key, std::function<void()> callback) override {
        return false;
    }
    
    bool register_stop_hotkey(const std::string& key, std::function<void()> callback) override {
        return false;
    }
    
    bool register_quit_hotkey(const std::string& key, std::function<void()> callback) override {
        return false;
    }
    
    void unregister_all_hotkeys() override {}
    
    bool is_supported() const override {
        return false;
    }
};

#endif

// Factory function
std::unique_ptr<HotkeyManager> create_hotkey_manager() {
#ifdef __APPLE__
    return std::make_unique<CarbonHotkeyManager>();
#else
    return std::make_unique<FallbackHotkeyManager>();
#endif
}

} // namespace SuperWhisper
