#pragma once

#include "superwhisper.hpp"
#include <memory>

namespace SuperWhisper {

// Factory function for creating GUI manager
std::unique_ptr<GuiManager> create_gui_manager();

} // namespace SuperWhisper
