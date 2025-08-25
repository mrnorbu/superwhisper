#pragma once

#include "superwhisper.hpp"
#include <memory>

namespace SuperWhisper {

// Factory function for creating audio recorder
std::unique_ptr<AudioRecorder> create_audio_recorder();

} // namespace SuperWhisper
