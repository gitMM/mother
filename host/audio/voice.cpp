#include "audio/voice.h"

#include <algorithm>
#include <cmath>

#include "audio/commands.h"

namespace mother {

namespace {

constexpr int32_t kEnvMax = 65536;
constexpr int32_t kFull = 32767;

// The C-0 phase step (16.3516 Hz at 48 kHz, times 2^32) and 2^(1/192). The table is built by
// repeated IEEE multiplication, which gives the same doubles on every platform.
constexpr double kC0Increment = 1463116.206723416;
constexpr double kSixteenthRatio = 1.0036166659754628;

const std::array<uint32_t, kPitchesPerOctave>& octave_table() {
    static const std::array<uint32_t, kPitchesPerOctave> table = [] {
        std::array<uint32_t, kPitchesPerOctave> t{};
        double v = kC0Increment;
        for (int i = 0; i < kPitchesPerOctave; ++i) {
            t[size_t(i)] = uint32_t(std::llround(v));
            v *= kSixteenthRatio;
        }
        return t;
    }();
    return table;
}

// Triangle LFO: phase 0-255 -> -64..63.
int lfo(uint8_t phase) {
    return phase < 128 ? phase - 64 : 191 - phase;
}

}  // namespace

uint32_t pitch_increment(int pitch) {
    pitch = std::clamp(pitch, 0, kMaxPitch);
    return octave_table()[size_t(pitch % kPitchesPerOctave)] << (pitch / kPitchesPerOctave);
}

void Voice::note_on(int pitch, const VoiceParams& params, bool music) {
    params_ = params;
    music_ = music;
    pitch_ = std::clamp(pitch, 0, kMaxPitch);
    phase_ = 0;
    noise_clock_ = 0;
    lfsr_ = 0x7FFF;
    vibrato_phase_ = 0;
    env_ = 0;
    enter(Stage::Attack);
}

void Voice::release() {
    if (stage_ != Stage::Off) enter(Stage::Release);
}

void Voice::stop() {
    env_ = 0;
    stage_ = Stage::Off;
}

void Voice::begin_tick() {
    if (stage_ == Stage::Off) return;
    pitch_ = std::clamp(pitch_ + params_.slide, 0, kMaxPitch);
    vibrato_phase_ = uint8_t(vibrato_phase_ + params_.vibrato_speed);
    const int wobble = params_.vibrato_depth * lfo(vibrato_phase_) / 64;
    increment_ = pitch_increment(pitch_ + wobble);
}

int32_t Voice::next(const std::array<Wavetable, kWavetables>& tables) {
    if (stage_ == Stage::Off) return 0;
    const int32_t level = int32_t((int64_t(oscillator(tables)) * env_) >> 16);
    advance_envelope();
    return (level * params_.volume) >> 8;  // volume / 64, then / 4 headroom for the mix
}

int32_t Voice::oscillator(const std::array<Wavetable, kWavetables>& tables) {
    const uint32_t phase = phase_;
    phase_ += increment_;
    switch (params_.wave) {
    case Wave::Square12: return phase < 0x20000000u ? kFull : -kFull;
    case Wave::Square25: return phase < 0x40000000u ? kFull : -kFull;
    case Wave::Square50: return phase < 0x80000000u ? kFull : -kFull;
    case Wave::Saw: return int32_t(phase >> 16) - 32768;
    case Wave::Triangle: {
        const int32_t p = int32_t(phase >> 15);  // 0..131071
        return p < 65536 ? p - 32768 : 98303 - p;
    }
    case Wave::Noise: {
        // A 15-bit LFSR clocked at 32x the note frequency.
        noise_clock_ += uint64_t(increment_) * 32;
        while (noise_clock_ >= (uint64_t(1) << 32)) {
            noise_clock_ -= uint64_t(1) << 32;
            const uint16_t bit = uint16_t((lfsr_ ^ (lfsr_ >> 1)) & 1);
            lfsr_ = uint16_t((lfsr_ >> 1) | (bit << 14));
        }
        return (lfsr_ & 1) ? kFull : -kFull;
    }
    case Wave::Table: return int32_t(tables[params_.table][phase >> 27]) * 256;
    }
    return 0;
}

void Voice::enter(Stage stage) {
    stage_ = stage;
    switch (stage) {
    case Stage::Attack:
        if (params_.attack == 0) {
            env_ = kEnvMax;
            enter(Stage::Decay);
        } else {
            stage_left_ = params_.attack * kTickFrames;
            env_step_ = (kEnvMax - env_) / stage_left_;
        }
        break;
    case Stage::Decay: {
        const int32_t target = params_.sustain * (kEnvMax / kMaxVolume);
        if (params_.decay == 0) {
            env_ = target;
            enter(Stage::Sustain);
        } else {
            stage_left_ = params_.decay * kTickFrames;
            env_step_ = (target - env_) / stage_left_;
        }
        break;
    }
    case Stage::Sustain:
        env_ = params_.sustain * (kEnvMax / kMaxVolume);
        if (env_ == 0) stage_ = Stage::Off;  // a percussive note has finished
        break;
    case Stage::Release:
        if (params_.release == 0 || env_ == 0) {
            stop();
        } else {
            stage_left_ = params_.release * kTickFrames;
            env_step_ = -env_ / stage_left_;
        }
        break;
    case Stage::Off: break;
    }
}

void Voice::advance_envelope() {
    if (stage_ != Stage::Attack && stage_ != Stage::Decay && stage_ != Stage::Release) return;
    env_ += env_step_;
    if (--stage_left_ > 0) return;
    if (stage_ == Stage::Attack) {
        env_ = kEnvMax;
        enter(Stage::Decay);
    } else if (stage_ == Stage::Decay) {
        enter(Stage::Sustain);
    } else {
        stop();
    }
}

}  // namespace mother
