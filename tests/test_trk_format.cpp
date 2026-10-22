#include <string>

#include "assets/trk_cell.h"
#include "assets/trk_format.h"
#include "test.h"

using namespace mother;

namespace {

const std::string kEmptyRow = "--- .. .. ... | --- .. .. ... | --- .. .. ... | --- .. .. ... | --- .. .. ... | --- .. .. ...\n";

std::string pattern_text(int n, const std::string& first_row, int rows = 64) {
    std::string text = "pattern " + std::to_string(n) + "\n" + first_row + "\n";
    for (int r = 1; r < rows; ++r) text += kEmptyRow;
    return text;
}

bool parse(const std::string& text, TrackerData& out, std::string& error) {
    return parse_trk(text, "x.trk", out, error);
}

bool fails_with(const std::string& text, const char* part) {
    TrackerData data;
    std::string error;
    if (parse(text, data, error)) {
        std::printf("  parsed, expected an error containing \"%s\"\n", part);
        return false;
    }
    if (error.find(part) != std::string::npos) return true;
    std::printf("  \"%s\" not found in: %s\n", part, error.c_str());
    return false;
}

}  // namespace

TEST(trkformat_cells_round_trip) {
    for (const char* text : {"C-4 01 40 ...", "C#0 3F 00 V42", "B-7 .. .. P80", "=== .. .. ...", "--- .. 20 D10",
                             "A-4 0A .. T05"}) {
        Cell cell;
        std::string error;
        CHECK(parse_cell(text, cell, error));
        CHECK_EQ(format_cell(cell), std::string(text));
    }
    Cell cell;
    std::string error;
    CHECK(parse_cell("C-4 01 40 ...", cell, error));
    CHECK_EQ(cell.note, 49);  // 4 * 12 + 0 + 1
    CHECK_EQ(cell.inst, 1);
    CHECK_EQ(cell.vol, 64);
    CHECK(parse_cell("B-7 .. .. ...", cell, error));
    CHECK_EQ(cell.note, 96);
    CHECK_EQ(cell.inst, -1);
}

TEST(trkformat_bad_cells) {
    Cell cell;
    std::string error;
    for (const char* text : {"C-8 01 40 ...", "E#4 01 40 ...", "H-4 01 40 ...", "C-4 40 40 ...", "C-4 01 41 ...",
                             "C-4 01 40 X00", "C-4 01 40", "C-4 1 40 ...", "c-4 01 40 ..."}) {
        if (parse_cell(text, cell, error)) std::printf("  accepted: %s\n", text);
        CHECK(!parse_cell(text, cell, error));
    }
}

TEST(trkformat_parses_every_block) {
    std::string wavetable = "wavetable 3\n-128 127";
    for (int i = 2; i < 32; ++i) wavetable += " 0";
    const std::string text = "# a comment\n" + wavetable + "\n" +
                             "\ninstrument 0A\nwave table 3\nadsr 1 2 32 4\nvolume 50\npan 10\nslide -5\nvibrato 8 12\n"
                             "\nsfx 7 3\nC-4 0A 40 ...\n# comment inside\n=== .. .. ...\n"
                             "\nfx\ndelay 100 20 30\nlowpass 128\nbitcrush 8 2\n\n" +
                             pattern_text(5, "C-4 0A 40 ... | --- .. .. ... | --- .. .. ... | --- .. .. ... | --- .. .. ... | D-2 01 .. U03") +
                             "\nsong 2 6 once\n5 5 0\n";
    TrackerData data;
    std::string error;
    CHECK(parse(text, data, error));
    if (!error.empty()) std::printf("  %s\n", error.c_str());
    CHECK_EQ(data.setup.wavetables[3][0], -128);
    CHECK_EQ(data.setup.wavetables[3][1], 127);
    const VoiceParams& p = data.instruments[10];
    CHECK(p.wave == Wave::Table && p.table == 3);
    CHECK(p.attack == 1 && p.decay == 2 && p.sustain == 32 && p.release == 4);
    CHECK(p.volume == 50 && p.pan == 10 && p.slide == -5 && p.vibrato_speed == 8 && p.vibrato_depth == 12);
    CHECK(data.instruments[0].wave == Wave::Square50);  // never defined: defaults
    CHECK_EQ(data.sfx[7].speed, 3);
    CHECK_EQ(data.sfx[7].rows.size(), size_t(2));
    CHECK_EQ(data.sfx[7].rows[1].note, kNoteOff);
    CHECK(data.setup.fx.delay_ms == 100 && data.setup.fx.delay_feedback == 20 && data.setup.fx.delay_mix == 30);
    CHECK(data.setup.fx.lowpass == 128 && data.setup.fx.crush_bits == 8 && data.setup.fx.crush_rate == 2);
    CHECK_EQ(data.patterns[5][0][0].note, 49);
    CHECK_EQ(data.patterns[5][0][5].fx, 'U');
    CHECK_EQ(data.patterns[5][0][5].param, 3);
    CHECK_EQ(data.songs[2].tempo, 6);
    CHECK(!data.songs[2].loop);
    CHECK(data.songs[2].order == std::vector<uint8_t>({5, 5, 0}));
    CHECK_EQ(data.songs[0].tempo, 0);
}

TEST(trkformat_errors_name_the_line) {
    CHECK(fails_with("instrument 40\n", "x.trk:1: expected `instrument XX`"));
    CHECK(fails_with("instrument 01\nwave sine\n", "x.trk:2: expected `wave"));
    CHECK(fails_with("instrument 01\nvolume 65\n", "x.trk:2: expected `volume V`"));
    CHECK(fails_with("instrument 01\npan 1\npan 2\n", "x.trk:3: `pan` is set twice"));
    CHECK(fails_with("instrument 01\ncolour 3\n", "unknown instrument key"));
    CHECK(fails_with("instrument 01\n\ninstrument 01\n", "x.trk:3: instrument 01 is defined twice"));
    CHECK(fails_with("sfx 0 0\nC-4 01 40 ...\n", "x.trk:1: expected `sfx N SPEED`"));
    CHECK(fails_with("sfx 0 4\n\n", "sfx 0 has no rows"));
    CHECK(fails_with("sfx 0 4\nC-4 01 40 ...\nC-9 01 40 ...\n", "x.trk:3: 'C-9' is not a note"));
    CHECK(fails_with(pattern_text(0, kEmptyRow.substr(0, kEmptyRow.size() - 1), 63), "pattern 0 has 63 rows"));
    CHECK(fails_with(pattern_text(0, "--- .. .. ... | --- .. .. ..."), "x.trk:2: expected 6 cells separated by |, found 2"));
    CHECK(fails_with(pattern_text(0, "--- .. .. ... | --- .. .. ... | --- .. .. ... | --- .. .. ... | --- .. .. ... | C-4 01 99 ..."),
                     "x.trk:2: channel 6: '99' is not a volume"));
    CHECK(fails_with("song 8 6 loop\n0\n", "x.trk:1: expected `song N TEMPO loop|once`"));
    CHECK(fails_with("song 0 6 forever\n0\n", "x.trk:1:"));
    CHECK(fails_with("song 0 6 loop\n\n", "song 0 needs its data on the next line"));
    CHECK(fails_with("song 0 6 loop\n0 64\n", "x.trk:2: '64' is not a pattern"));
    CHECK(fails_with("wavetable 0\n1 2 3\n", "x.trk:2: expected 32 samples, found 3"));
    CHECK(fails_with("fx\ndelay 2000 0 0\n", "x.trk:2: expected `delay MS FEEDBACK MIX`"));
    CHECK(fails_with("fx\nreverb 3\n", "unknown fx key"));
    CHECK(fails_with("tune 1\n", "x.trk:1: expected a block"));
}

TEST(trkformat_empty_file_is_valid) {
    TrackerData data;
    std::string error;
    CHECK(parse("", data, error));
    CHECK(parse("# nothing\n\n", data, error));
}
