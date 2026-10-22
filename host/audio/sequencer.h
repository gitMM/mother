#pragma once

#include <array>
#include <cstdint>

#include "audio/commands.h"

namespace mother {

constexpr int kRowsPerBeat = 4;
constexpr int kBeatsPerBar = 4;

// The simulation side of audio: plays songs and sfx from the tracker data on the 60 Hz tick
// and turns them into synth commands. It runs on the simulation thread, so the music position
// that beat() reports is deterministic. Music channel c plays on voice c (0-5); an sfx takes
// over its voice until it ends, then the music channel speaks again at its next note.
class Sequencer {
public:
    explicit Sequencer(TrackerData& data) : data_(&data) {}

    // Cart API; the bindings validate arguments.
    void sfx(int n, int voice);             // voice 0-5, or -1 to pick one
    void music(int song, int fade_ticks);   // song -1 stops, fading out over fade_ticks
    void stop(int voice);                   // voice 0-5, or -1 for all music and sfx
    // The current music position: bars and beats since the song started (16 rows a bar, 4 a
    // beat) and the row in the current pattern. False when no music is playing.
    bool beat(int& bar, int& beat, int& row) const;

    // One tick, after cart.update: appends the commands from this tick's API calls, fades,
    // music rows and sfx rows to out, in that order.
    void tick(TickAudio& out);

private:
    struct SfxPlay {
        int n = -1;  // -1 when the voice has no sfx
        int row = 0;
        int wait = 0;
        int inst = 0;
        uint64_t order = 0;  // start order, for replacing the oldest
    };

    void trigger(const Cell& cell, int voice, bool music, int& inst, TickAudio& out) const;
    void music_tick(TickAudio& out);
    void sfx_tick(TickAudio& out);
    void end_music(TickAudio& out);

    TrackerData* data_;
    TickAudio pending_;  // commands from API calls since the last tick

    int song_ = -1;
    int position_ = 0;  // index into the song's pattern order
    int next_row_ = 0;
    int wait_ = 0;      // ticks until the next row
    int tempo_ = 1;
    bool last_row_ = false;  // a "once" song is on its final row
    int rows_played_ = 0;
    int row_ = 0;            // row of the last triggered row
    std::array<int, kChannels> music_inst_{};

    int gain_ = kMusicGainMax;
    int fade_from_ = 0, fade_to_ = 0, fade_len_ = 0, fade_t_ = 0;
    bool stop_after_fade_ = false;

    std::array<SfxPlay, kCartVoices> sfx_{};
    uint64_t sfx_started_ = 0;
};

}  // namespace mother
