#pragma once

#include "superwhisper.hpp"
#include <memory>

namespace SuperWhisper {

// Factory function for creating Whisper wrapper
std::unique_ptr<WhisperWrapper> create_whisper_wrapper();

} // namespace SuperWhisper
