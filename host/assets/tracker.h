#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace mother {

constexpr int kVoices = 8;          // voices 7 and 8 (indices 6, 7) are reserved for imports and the host
constexpr int kCartVoices = 6;      // voices 1-6: the music channels, and where carts play sfx
constexpr int kChannels = 6;        // music channels per pattern
constexpr int kPatterns = 64;
constexpr int kPatternRows = 64;
constexpr int kSongs = 8;
constexpr int kSongLength = 64;     // pattern indices per song
constexpr int kInstruments = 64;
constexpr int kSfxCount = 64;
constexpr int kSfxRows = 64;
constexpr int kWavetables = 32;
constexpr int kWavetableSize = 32;
constexpr int kMaxVolume = 64;
constexpr int kNoteCount = 96;      // C-0 .. B-7
constexpr int kNoteOff = -1;        // Cell::note value for "==="

enum class Wave : uint8_t { Square12, Square25, Square50, Saw, Triangle, Noise, Table };

// Everything a note starts with. Instruments hold the defaults; row volume and effects override.
struct VoiceParams {
    Wave wave = Wave::Square50;
    uint8_t table = 0;                         // wavetable for Wave::Table
    uint8_t attack = 0, decay = 0;             // ticks
    uint8_t sustain = kMaxVolume;              // level 0-64
    uint8_t release = 0;                       // ticks
    uint8_t volume = kMaxVolume;               // 0-64
    uint8_t pan = 128;                         // 0 left .. 255 right, 128 centre
    int8_t slide = 0;                          // sixteenths of a semitone per tick
    uint8_t vibrato_speed = 0, vibrato_depth = 0;  // LFO steps (of 256) per tick; sixteenths of a semitone
};

// One tracker cell: note, instrument, volume, effect.
struct Cell {
    int8_t note = 0;     // 0 empty, 1-96 = C-0..B-7, kNoteOff
    int8_t inst = -1;    // -1 none, else 0-63
    int8_t vol = -1;     // -1 none, else 0-64
    char fx = 0;         // 0 none, else 'P' 'U' 'D' 'V' 'T'
    uint8_t param = 0;   // effect parameter, 00-FF
};

using PatternRow = std::array<Cell, kChannels>;
using Pattern = std::array<PatternRow, kPatternRows>;

struct Sfx {
    int speed = 0;            // ticks per row; 0 = not defined
    std::vector<Cell> rows;   // 1-64
};

struct Song {
    int tempo = 0;                 // ticks per row; 0 = not defined
    bool loop = true;
    std::vector<uint8_t> order;    // pattern indices, 1-64
};

struct GlobalFx {
    int delay_ms = 0;          // 0-1000
    int delay_feedback = 0;    // 0-64
    int delay_mix = 0;         // 0-64; 0 = no delay
    int lowpass = 255;         // cutoff 0-255; 255 = open
    int crush_bits = 16;       // 1-16; 16 = off
    int crush_rate = 1;        // sample-and-hold period 1-16; 1 = off
};

using Wavetable = std::array<int8_t, kWavetableSize>;

// What the synth needs besides the per-tick commands; fixed for a cart's whole run.
struct AudioSetup {
    std::array<Wavetable, kWavetables> wavetables{};
    GlobalFx fx;
};

// The contents of a cart's audio.trk. Patterns are writable at run time (song_set).
struct TrackerData {
    std::array<VoiceParams, kInstruments> instruments{};
    std::vector<Pattern> patterns = std::vector<Pattern>(kPatterns);
    std::array<Sfx, kSfxCount> sfx{};
    std::array<Song, kSongs> songs{};
    AudioSetup setup;
};

}  // namespace mother
