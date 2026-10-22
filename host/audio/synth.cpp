#include "audio/synth.h"

#include <algorithm>

namespace mother {

namespace {

constexpr int32_t kDelayLimit = 131071;  // keeps feedback from running away

int16_t clamp16(int32_t v) {
    return int16_t(std::clamp(v, -32768, 32767));
}

}  // namespace

Synth::Synth(const AudioSetup& setup) : setup_(setup) {
    const GlobalFx& fx = setup_.fx;
    if (fx.delay_mix > 0 && fx.delay_ms > 0) delay_.assign(size_t(fx.delay_ms) * (kSampleRate / 1000) * 2, 0);
}

void Synth::render_tick(const TickAudio& commands, int16_t* out) {
    for (const AudioCommand& c : commands) apply(c);
    for (Voice& v : voices_) v.begin_tick();
    for (int i = 0; i < kTickFrames; ++i) {
        int32_t left = 0, right = 0;
        for (Voice& v : voices_) {
            if (!v.active()) continue;
            int32_t s = v.next(setup_.wavetables);
            if (v.music()) s = int32_t((int64_t(s) * music_gain_) >> 16);
            left += (s * (256 - v.pan())) >> 8;
            right += (s * v.pan()) >> 8;
        }
        effects(left, right);
        out[2 * i] = clamp16(left);
        out[2 * i + 1] = clamp16(right);
    }
}

void Synth::apply(const AudioCommand& c) {
    Voice& v = voices_[c.voice % kVoices];
    switch (c.kind) {
    case AudioCommand::Kind::NoteOn: v.note_on(c.pitch, c.params, c.music); break;
    case AudioCommand::Kind::Release: v.release(); break;
    case AudioCommand::Kind::Stop: v.stop(); break;
    case AudioCommand::Kind::Volume: v.set_volume(c.value); break;
    case AudioCommand::Kind::Pan: v.set_pan(c.value); break;
    case AudioCommand::Kind::Slide: v.set_slide(c.value); break;
    case AudioCommand::Kind::Vibrato: v.set_vibrato(c.value >> 8, c.value & 0xFF); break;
    case AudioCommand::Kind::MusicGain: music_gain_ = std::clamp(c.value, 0, kMusicGainMax); break;
    }
}

void Synth::effects(int32_t& left, int32_t& right) {
    const GlobalFx& fx = setup_.fx;
    if (fx.crush_rate > 1) {
        if (crush_count_ == 0) crush_left_ = left, crush_right_ = right;
        crush_count_ = (crush_count_ + 1) % fx.crush_rate;
        left = crush_left_, right = crush_right_;
    }
    if (fx.crush_bits < 16) {
        const int shift = 16 - fx.crush_bits;
        left = (left >> shift) << shift;
        right = (right >> shift) << shift;
    }
    if (fx.lowpass < 255) {
        const int64_t k = int64_t(fx.lowpass + 1) * 256;  // 256..65280 of 65536
        low_left_ += int32_t((int64_t(left - low_left_) * k) >> 16);
        low_right_ += int32_t((int64_t(right - low_right_) * k) >> 16);
        left = low_left_, right = low_right_;
    }
    if (!delay_.empty()) {
        const int32_t echo_left = delay_[delay_pos_], echo_right = delay_[delay_pos_ + 1];
        delay_[delay_pos_] = std::clamp(left + ((echo_left * fx.delay_feedback) >> 6), -kDelayLimit, kDelayLimit);
        delay_[delay_pos_ + 1] = std::clamp(right + ((echo_right * fx.delay_feedback) >> 6), -kDelayLimit, kDelayLimit);
        delay_pos_ = (delay_pos_ + 2) % delay_.size();
        left += (echo_left * fx.delay_mix) >> 6;
        right += (echo_right * fx.delay_mix) >> 6;
    }
}

}  // namespace mother
