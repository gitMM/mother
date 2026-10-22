#pragma once

#include <cstdint>
#include <vector>

#include "assets/tracker.h"

namespace mother {

constexpr int kSampleRate = 48000;
constexpr int kTickFrames = kSampleRate / 60;  // 800 stereo frames per 60 Hz tick
constexpr int kMusicGainMax = 65536;

// What the simulation tells the synth during one tick. A tick's commands apply in order at the
// start of the tick, before its 800 frames are rendered, so the samples depend only on the
// command stream, never on when or on which thread they are rendered.
struct AudioCommand {
    enum class Kind : uint8_t { NoteOn, Release, Stop, Volume, Pan, Slide, Vibrato, MusicGain };

    Kind kind = Kind::Stop;
    uint8_t voice = 0;    // 0-7
    bool music = false;   // NoteOn: a music note, scaled by the music gain
    int16_t pitch = 0;    // NoteOn: sixteenths of a semitone above C-0
    VoiceParams params;   // NoteOn
    int32_t value = 0;    // Volume 0-64, Pan 0-255, Slide -128..127, Vibrato speed << 8 | depth,
                          // MusicGain 0-65536
};

using TickAudio = std::vector<AudioCommand>;

}  // namespace mother
