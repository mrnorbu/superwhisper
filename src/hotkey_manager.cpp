#include "hotkey_manager.hpp"
#include <iostream>

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#endif

namespace SuperWhisper {

class CarbonHotkeyManager : public HotkeyManager {
public:
    CarbonHotkeyManager() : hotkey_ref_(0), callback_(nullptr) {}
    
    ~CarbonHotkeyManager() override {
        unregister_hotkey();
    }
    
    bool register_hotkey(int key_code) override {
#ifdef __APPLE__
        try {
            // Convert key code to Carbon key code
            UInt32 carbon_key_code = 0;
            switch (key_code) {
                case 0x3F: // F9
                    carbon_key_code = kVK_F9;
                    break;
                default:
                    std::cerr << "Unsupported key code: " << key_code << std::endl;
                    return false;
            }
            
            // Register hotkey
            EventHotKeyID hotkey_id;
            hotkey_id.signature = 'swhk';  // SuperWhisper hotkey
            hotkey_id.id = 1;
            
            OSStatus status = RegisterEventHotKey(
                carbon_key_code,
                cmdKey,  // Command key modifier
                hotkey_id,
                GetApplicationEventTarget(),
                0,
                &hotkey_ref_
            );
            
            if (status != noErr) {
                std::cerr << "Failed to register hotkey: " << status << std::endl;
                return false;
            }
            
            // Install event handler
            EventTypeSpec event_type;
            event_type.eventClass = kEventClassKeyboard;
            event_type.eventKind = kEventHotKeyPressed;
            
            status = InstallEventHandler(
                GetApplicationEventTarget(),
                &CarbonHotkeyManager::hotkey_event_handler,
                1,
                &event_type,
                this,
                &event_handler_ref_
            );
            
            if (status != noErr) {
                std::cerr << "Failed to install event handler: " << status << std::endl;
                UnregisterEventHotKey(hotkey_ref_);
                hotkey_ref_ = 0;
                return false;
            }
            
            std::cout << "F9 hotkey registered successfully" << std::endl;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Hotkey registration failed: " << e.what() << std::endl;
            return false;
        }
#else
        std::cerr << "Global hotkeys not supported on this platform" << std::endl;
        return false;
#endif
    }
    
    void unregister_hotkey() override {
#ifdef __APPLE__
        if (hotkey_ref_) {
            UnregisterEventHotKey(hotkey_ref_);
            hotkey_ref_ = 0;
        }
        if (event_handler_ref_) {
            RemoveEventHandler(event_handler_ref_);
            event_handler_ref_ = nullptr;
        }
#endif
    }
    
    void set_callback(std::function<void()> callback) override {
        callback_ = callback;
    }
    
private:
#ifdef __APPLE__
    // Carbon event handler for hotkey
    static OSStatus hotkey_event_handler(
        EventHandlerCallRef next_handler,
        EventRef event,
        void* user_data
    ) {
        auto* manager = static_cast<CarbonHotkeyManager*>(user_data);
        
        if (manager && manager->callback_) {
            // Execute callback in main thread
            manager->callback_();
        }
        
        return noErr;
    }
    
    EventHotKeyRef hotkey_ref_;
    EventHandlerRef event_handler_ref_;
#endif
    
    std::function<void()> callback_;
};

// Factory function
std::unique_ptr<HotkeyManager> create_hotkey_manager() {
    return std::make_unique<CarbonHotkeyManager>();
}

} // namespace SuperWhisper
