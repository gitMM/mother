#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "audio/commands.h"
#include "audio/voice.h"

namespace mother {

// The 8-voice synth with global delay, low-pass and bitcrush (brief section 3), in integer
// arithmetic. Fed one tick at a time; the output depends only on the setup and the commands.
class Synth {
public:
    explicit Synth(const AudioSetup& setup);

    // Applies a tick's commands, then renders its kTickFrames stereo frames into out
    // (interleaved left, right; kTickFrames * 2 samples).
    void render_tick(const TickAudio& commands, int16_t* out);

private:
    void apply(const AudioCommand& c);
    void effects(int32_t& left, int32_t& right);

    AudioSetup setup_;
    std::array<Voice, kVoices> voices_;
    int32_t music_gain_ = kMusicGainMax;
    int crush_count_ = 0;
    int32_t crush_left_ = 0, crush_right_ = 0;
    int32_t low_left_ = 0, low_right_ = 0;
    std::vector<int32_t> delay_;  // interleaved stereo ring
    size_t delay_pos_ = 0;
};

}  // namespace mother
