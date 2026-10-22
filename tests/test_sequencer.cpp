#include <string>
#include <vector>

#include "assets/trk_cell.h"
#include "audio/sequencer.h"
#include "test.h"

using namespace mother;
using Kind = AudioCommand::Kind;

namespace {

Cell cell(const char* text) {
    Cell c;
    std::string error;
    if (!parse_cell(text, c, error)) std::printf("  bad test cell %s: %s\n", text, error.c_str());
    return c;
}

// Song 0: tempo 3, pattern 0 then 1, looping. Row r of pattern 0, channel 1 holds a note.
TrackerData make_data() {
    TrackerData d;
    d.songs[0].tempo = 3;
    d.songs[0].order = {0, 1};
    d.patterns[0][0][0] = cell("C-4 01 40 ...");
    d.patterns[0][1][0] = cell("D-4 .. 20 ...");
    d.patterns[0][2][0] = cell("=== .. .. ...");
    d.patterns[0][3][1] = cell("--- .. 10 P20");
    d.instruments[1].wave = Wave::Saw;
    d.sfx[0].speed = 2;
    d.sfx[0].rows = {cell("C-5 02 40 ..."), cell("E-5 .. .. ...")};
    return d;
}

std::vector<TickAudio> run(Sequencer& seq, int ticks) {
    std::vector<TickAudio> out(static_cast<size_t>(ticks));
    for (auto& t : out) seq.tick(t);
    return out;
}

int count(const TickAudio& t, Kind kind, int voice = -1) {
    int n = 0;
    for (const AudioCommand& c : t) n += c.kind == kind && (voice < 0 || c.voice == voice) ? 1 : 0;
    return n;
}

const AudioCommand* find(const TickAudio& t, Kind kind, int voice) {
    for (const AudioCommand& c : t) {
        if (c.kind == kind && c.voice == voice) return &c;
    }
    return nullptr;
}

}  // namespace

TEST(sequencer_plays_rows_at_the_tempo) {
    TrackerData d = make_data();
    Sequencer seq(d);
    seq.music(0, 0);
    const std::vector<TickAudio> t = run(seq, 12);
    const AudioCommand* first = find(t[0], Kind::NoteOn, 0);
    CHECK(first != nullptr);
    if (first) {
        CHECK_EQ(first->pitch, (49 - 1) * 16);
        CHECK(first->music);
        CHECK(first->params.wave == Wave::Saw);
        CHECK_EQ(first->params.volume, 64);
    }
    CHECK_EQ(count(t[1], Kind::NoteOn) + count(t[2], Kind::NoteOn), 0);
    const AudioCommand* second = find(t[3], Kind::NoteOn, 0);
    CHECK(second && second->params.wave == Wave::Saw && second->params.volume == 32);  // keeps instrument 01
    CHECK_EQ(count(t[6], Kind::Release, 0), 1);
    CHECK_EQ(count(t[9], Kind::Volume, 1), 1);
    const AudioCommand* pan = find(t[9], Kind::Pan, 1);
    CHECK(pan && pan->value == 0x20);
}

TEST(sequencer_beat_counts_rows_bars_and_patterns) {
    TrackerData d = make_data();
    Sequencer seq(d);
    int bar = -1, beat = -1, row = -1;
    CHECK(!seq.beat(bar, beat, row));
    seq.music(0, 0);
    CHECK(seq.beat(bar, beat, row));
    CHECK(bar == 0 && beat == 0 && row == 0);
    run(seq, 3 * 21);  // rows 0-20 triggered
    seq.beat(bar, beat, row);
    CHECK(bar == 1 && beat == 1 && row == 20);
    run(seq, 3 * 50);  // 71 rows: pattern 1, row 6
    seq.beat(bar, beat, row);
    CHECK(bar == 4 && beat == 1 && row == 6);
}

TEST(sequencer_tempo_effect_and_once_songs) {
    TrackerData d = make_data();
    d.patterns[0][0][5] = cell("--- .. .. T01");
    d.songs[1] = {5, false, {0}};
    Sequencer seq(d);
    seq.music(1, 0);
    run(seq, 64);  // tempo 1 from row 0 on: one row a tick, the whole pattern
    int bar = 0, beat = 0, row = 0;
    CHECK(!seq.beat(bar, beat, row));  // a "once" song stops after its last row
}

TEST(sequencer_sfx_takes_a_voice_until_it_ends) {
    TrackerData d = make_data();
    Sequencer seq(d);
    seq.music(0, 0);
    seq.sfx(0, -1);  // voice 0 is free of sfx, so it goes there
    std::vector<TickAudio> t = run(seq, 6);
    const AudioCommand* blip = find(t[0], Kind::NoteOn, 0);
    CHECK(blip && !blip->music && blip->pitch == (61 - 1) * 16);
    CHECK_EQ(count(t[0], Kind::NoteOn, 0), 1);  // the music note on channel 1 is held back
    CHECK_EQ(count(t[2], Kind::NoteOn, 0), 1);  // second sfx row
    CHECK_EQ(count(t[3], Kind::NoteOn, 0), 0);  // music row 1 still held back
    CHECK_EQ(count(t[4], Kind::Release, 0), 1);  // sfx over
    t = run(seq, 3);
    CHECK_EQ(count(t[0], Kind::Release, 0), 1);  // music speaks again: row 2 is a note off
}

TEST(sequencer_sfx_voice_choice) {
    TrackerData d = make_data();
    Sequencer seq(d);
    for (int i = 0; i < 6; ++i) seq.sfx(0, -1);
    std::vector<TickAudio> t = run(seq, 1);
    for (int v = 0; v < 6; ++v) CHECK_EQ(count(t[0], Kind::NoteOn, v), 1);
    seq.sfx(0, -1);  // all busy: replaces the oldest, voice 0
    seq.sfx(0, 4);
    t = run(seq, 1);
    CHECK_EQ(count(t[0], Kind::NoteOn), 2);
    CHECK_EQ(count(t[0], Kind::NoteOn, 0), 1);
    CHECK_EQ(count(t[0], Kind::NoteOn, 4), 1);
}

TEST(sequencer_stop_and_fades) {
    TrackerData d = make_data();
    Sequencer seq(d);
    seq.music(0, 4);
    std::vector<TickAudio> t = run(seq, 4);
    const AudioCommand* g0 = find(t[0], Kind::MusicGain, 0);
    CHECK(g0 && g0->value == 0);  // music() starts silent...
    int last = 0;
    for (const AudioCommand& c : t[3]) {
        if (c.kind == Kind::MusicGain) last = c.value;
    }
    CHECK_EQ(last, kMusicGainMax);  // ...and is at full volume after 4 ticks
    seq.music(-1, 2);
    t = run(seq, 2);
    int bar = 0, beat = 0, row = 0;
    CHECK(!seq.beat(bar, beat, row));
    CHECK_EQ(count(t[1], Kind::Release), 6);

    seq.music(0, 0);
    seq.sfx(0, 2);
    seq.stop(2);
    t = run(seq, 1);
    CHECK_EQ(count(t[0], Kind::Stop, 2), 1);
    CHECK_EQ(count(t[0], Kind::NoteOn, 2), 0);  // the sfx was stopped before it began
    seq.stop(-1);
    t = run(seq, 1);
    CHECK_EQ(count(t[0], Kind::Stop), 6);
    CHECK(!seq.beat(bar, beat, row));
}

TEST(sequencer_reads_patterns_as_they_are_changed) {
    TrackerData d = make_data();
    Sequencer seq(d);
    seq.music(0, 0);
    run(seq, 3);
    d.patterns[0][1][0] = cell("G-4 .. .. ...");  // song_set on the next row
    const std::vector<TickAudio> t = run(seq, 1);
    const AudioCommand* n = find(t[0], Kind::NoteOn, 0);
    CHECK(n && n->pitch == (56 - 1) * 16);
}
