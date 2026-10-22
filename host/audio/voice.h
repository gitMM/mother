#pragma once

#include <array>
#include <cstdint>

#include "assets/tracker.h"

namespace mother {

constexpr int kPitchesPerOctave = 12 * 16;            // sixteenths of a semitone
constexpr int kMaxPitch = 9 * kPitchesPerOctave - 1;  // B-8 and a bit; slides stop here

// Oscillator phase step per sample at 48 kHz for a pitch in sixteenths of a semitone above C-0.
uint32_t pitch_increment(int pitch);

// One synth voice: oscillator, ADSR envelope, pitch slide and vibrato. All integer arithmetic,
// so every platform and build renders identical samples.
class Voice {
public:
    void note_on(int pitch, const VoiceParams& params, bool music);
    void release();
    void stop();
    void set_volume(int volume) { params_.volume = uint8_t(volume); }
    void set_pan(int pan) { params_.pan = uint8_t(pan); }
    void set_slide(int slide) { params_.slide = int8_t(slide); }
    void set_vibrato(int speed, int depth) { params_.vibrato_speed = uint8_t(speed), params_.vibrato_depth = uint8_t(depth); }

    bool active() const { return stage_ != Stage::Off; }
    bool music() const { return music_; }
    int pan() const { return params_.pan; }

    // Applies one tick of slide and vibrato and fixes the oscillator rate for the tick's frames.
    void begin_tick();
    // The next mono sample with envelope and volume applied, within about +-8192.
    int32_t next(const std::array<Wavetable, kWavetables>& tables);

private:
    enum class Stage : uint8_t { Off, Attack, Decay, Sustain, Release };

    int32_t oscillator(const std::array<Wavetable, kWavetables>& tables);
    void enter(Stage stage);
    void advance_envelope();

    VoiceParams params_;
    bool music_ = false;
    int pitch_ = 0;
    uint32_t phase_ = 0;
    uint32_t increment_ = 0;
    uint64_t noise_clock_ = 0;
    uint16_t lfsr_ = 0x7FFF;
    uint8_t vibrato_phase_ = 0;
    Stage stage_ = Stage::Off;
    int32_t env_ = 0;         // 0..kEnvMax
    int32_t env_step_ = 0;    // per sample
    int32_t stage_left_ = 0;  // samples left in Attack, Decay or Release
};

}  // namespace mother
