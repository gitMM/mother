#include <array>
#include <cstdlib>
#include <vector>

#include "audio/synth.h"
#include "test.h"

using namespace mother;

namespace {

using Samples = std::vector<int16_t>;

AudioCommand note_on(int voice, int note, VoiceParams p = {}, bool music = false) {
    AudioCommand c;
    c.kind = AudioCommand::Kind::NoteOn;
    c.voice = uint8_t(voice);
    c.pitch = int16_t((note - 1) * 16);
    c.params = p;
    c.music = music;
    return c;
}

AudioCommand command(AudioCommand::Kind kind, int voice, int value = 0) {
    AudioCommand c;
    c.kind = kind;
    c.voice = uint8_t(voice);
    c.value = value;
    return c;
}

// Renders ticks; ticks[i] holds the commands for tick i, missing ticks have none.
Samples render(Synth& synth, const std::vector<TickAudio>& ticks, int count) {
    Samples out;
    std::array<int16_t, kTickFrames * 2> buf{};
    for (int t = 0; t < count; ++t) {
        synth.render_tick(t < int(ticks.size()) ? ticks[size_t(t)] : TickAudio{}, buf.data());
        out.insert(out.end(), buf.begin(), buf.end());
    }
    return out;
}

int peak(const Samples& s, size_t from, size_t to, int channel) {
    int p = 0;
    for (size_t i = from * 2 + size_t(channel); i < to * 2 && i < s.size(); i += 2) p = std::max(p, std::abs(int(s[i])));
    return p;
}

int crossings(const Samples& s, size_t from, size_t to) {
    int n = 0;
    for (size_t f = from + 1; f < to; ++f) n += (s[f * 2] >= 0) != (s[(f - 1) * 2] >= 0) ? 1 : 0;
    return n;
}

}  // namespace

constexpr int kA4 = 4 * 12 + 9 + 1;

TEST(synth_is_silent_without_notes) {
    Synth synth(AudioSetup{});
    const Samples s = render(synth, {}, 3);
    CHECK_EQ(peak(s, 0, 2400, 0), 0);
    CHECK_EQ(peak(s, 0, 2400, 1), 0);
}

TEST(synth_pitch_table_is_tuned) {
    const int64_t a4 = pitch_increment((kA4 - 1) * 16);
    CHECK(std::llabs(a4 - 39370534) < 16);  // 440 Hz
    CHECK_EQ(pitch_increment((kA4 - 1 + 12) * 16), uint32_t(a4) * 2);
    CHECK_EQ(pitch_increment(-50), pitch_increment(0));
    CHECK_EQ(pitch_increment(1 << 20), pitch_increment(kMaxPitch));
}

TEST(synth_square_plays_at_the_note_frequency) {
    Synth synth(AudioSetup{});
    const Samples s = render(synth, {{note_on(0, kA4)}}, 60);
    const int n = crossings(s, 0, 48000);  // one second
    CHECK(n >= 878 && n <= 882);           // 440 Hz: two crossings a cycle
    CHECK(peak(s, 0, 48000, 0) > 3000);
}

TEST(synth_every_wave_sounds) {
    for (Wave w : {Wave::Square12, Wave::Square25, Wave::Square50, Wave::Saw, Wave::Triangle, Wave::Noise, Wave::Table}) {
        AudioSetup setup;
        setup.wavetables[0].fill(127);
        VoiceParams p;
        p.wave = w;
        Synth synth(setup);
        const Samples s = render(synth, {{note_on(0, kA4, p)}}, 2);
        CHECK(peak(s, 0, 1600, 0) > 2000);
    }
}

TEST(synth_adsr_shapes_the_volume) {
    VoiceParams p;
    p.attack = 10, p.decay = 10, p.sustain = 16, p.release = 5;
    Synth synth(AudioSetup{});
    std::vector<TickAudio> ticks(41);
    ticks[0] = {note_on(0, kA4, p)};
    ticks[30] = {command(AudioCommand::Kind::Release, 0)};
    const Samples s = render(synth, ticks, 40);
    const int start = peak(s, 0, 800, 0), top = peak(s, 9 * 800, 10 * 800, 0);
    const int held = peak(s, 25 * 800, 26 * 800, 0), released = peak(s, 36 * 800, 40 * 800, 0);
    CHECK(start < top / 4);
    CHECK(held > top / 5 && held < top / 3);  // sustain 16 of 64
    CHECK_EQ(released, 0);
}

TEST(synth_pan_and_music_gain) {
    VoiceParams left;
    left.pan = 0;
    Synth synth(AudioSetup{});
    Samples s = render(synth, {{note_on(0, kA4, left)}}, 1);
    CHECK(peak(s, 0, 800, 0) > 3000);
    CHECK_EQ(peak(s, 0, 800, 1), 0);

    Synth muted(AudioSetup{});
    s = render(muted, {{command(AudioCommand::Kind::MusicGain, 0, 0), note_on(0, kA4, {}, true), note_on(1, kA4 + 7)}}, 1);
    Synth sfx_only(AudioSetup{});
    const Samples alone = render(sfx_only, {{note_on(1, kA4 + 7)}}, 1);
    CHECK(s == alone);  // the music voice is silent at gain 0; the sfx voice is not
}

TEST(synth_slide_and_stop) {
    VoiceParams p;
    p.slide = 16;  // a semitone a tick
    Synth synth(AudioSetup{});
    std::vector<TickAudio> ticks(13);
    ticks[0] = {note_on(0, kA4, p)};
    ticks[12] = {command(AudioCommand::Kind::Stop, 0)};
    const Samples s = render(synth, ticks, 13);
    CHECK(crossings(s, 11 * 800, 12 * 800) > crossings(s, 0, 800) * 3 / 2);  // an octave up
    CHECK_EQ(peak(s, 12 * 800, 13 * 800, 0), 0);
}

TEST(synth_global_effects) {
    AudioSetup crush;
    crush.fx.crush_bits = 4;
    Synth crushed(crush);
    Samples s = render(crushed, {{note_on(0, kA4, {.wave = Wave::Saw})}}, 1);
    bool quantised = true;
    for (int16_t v : s) quantised = quantised && (v % 4096 == 0);
    CHECK(quantised);

    AudioSetup low;
    low.fx.lowpass = 4;
    Synth dark(low), bright(AudioSetup{});
    VoiceParams high;
    high.wave = Wave::Square50;
    CHECK(peak(render(dark, {{note_on(0, 85, high)}}, 4), 1600, 3200, 0) <
          peak(render(bright, {{note_on(0, 85, high)}}, 4), 1600, 3200, 0) / 2);

    AudioSetup echo;
    echo.fx.delay_ms = 100, echo.fx.delay_mix = 64, echo.fx.delay_feedback = 0;
    Synth delayed(echo);
    VoiceParams blip;
    blip.sustain = 0, blip.decay = 1;  // one tick of sound
    s = render(delayed, {{note_on(0, kA4, blip)}}, 12);
    CHECK(peak(s, 0, 800, 0) > 2000);
    CHECK_EQ(peak(s, 1000, 4800, 0), 0);    // silence until the echo
    CHECK(peak(s, 4800, 5600, 0) > 1000);   // 100 ms later
}

TEST(synth_renders_identically_every_time) {
    AudioSetup setup;
    setup.fx.delay_ms = 50, setup.fx.delay_mix = 30, setup.fx.delay_feedback = 40, setup.fx.lowpass = 200;
    VoiceParams noise;
    noise.wave = Wave::Noise, noise.vibrato_speed = 20, noise.vibrato_depth = 30;
    const std::vector<TickAudio> ticks = {{note_on(0, kA4, noise), note_on(3, 30)}, {}, {command(AudioCommand::Kind::Pan, 3, 20)}};
    Synth a(setup), b(setup);
    CHECK(render(a, ticks, 30) == render(b, ticks, 30));
}
