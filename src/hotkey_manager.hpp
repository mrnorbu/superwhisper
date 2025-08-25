#pragma once

#include "superwhisper.hpp"
#include <memory>

namespace SuperWhisper {

// Factory function for creating hotkey manager
std::unique_ptr<HotkeyManager> create_hotkey_manager();

} // namespace SuperWhisper
