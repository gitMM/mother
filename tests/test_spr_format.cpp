#include <string>

#include "assets/spr_format.h"
#include "test.h"

using namespace mother;

namespace {

std::string sheet_text(int n, char fill, int rows = 256, int width = 256) {
    std::string text = "sheet " + std::to_string(n) + "\n";
    for (int y = 0; y < rows; ++y) text += std::string(size_t(width), fill) + "\n";
    return text;
}

bool parse(const std::string& text, SprData& out, std::string& error) {
    return parse_spr(text, "x.spr", out, error);
}

bool fails_with(const std::string& text, const char* part) {
    SprData data;
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

TEST(sprformat_alphabet_is_base64) {
    CHECK_EQ(spr_index('A'), 0);
    CHECK_EQ(spr_index('Z'), 25);
    CHECK_EQ(spr_index('a'), 26);
    CHECK_EQ(spr_index('z'), 51);
    CHECK_EQ(spr_index('0'), 52);
    CHECK_EQ(spr_index('9'), 61);
    CHECK_EQ(spr_index('+'), 62);
    CHECK_EQ(spr_index('/'), 63);
    CHECK_EQ(spr_index('#'), -1);
    CHECK_EQ(spr_index('='), -1);
    for (int i = 0; i < 64; ++i) CHECK_EQ(spr_index(spr_char(uint8_t(i))), i);
}

TEST(sprformat_parses_sheets_and_ramps) {
    SprData data;
    std::string error;
    CHECK(parse(sheet_text(1, 'B') + "\nramp 3 1 2 63\n", data, error));
    CHECK(!data.defined[0] && data.defined[1] && !data.defined[2]);
    CHECK_EQ(data.sheets.get(0, 256), 1);
    CHECK_EQ(data.sheets.get(255, 511), 1);
    CHECK_EQ(data.sheets.get(0, 0), 0);
    CHECK_EQ(data.sheets.get(0, 512), 0);
    CHECK_EQ(data.ramps[3].size(), size_t(3));
    CHECK(data.ramps[3] == std::vector<uint8_t>({1, 2, 63}));
    CHECK(data.ramps[0].empty());
}

TEST(sprformat_write_then_parse_round_trips) {
    SprData data;
    data.defined[0] = data.defined[2] = true;
    for (int i = 0; i < 64; ++i) data.sheets.set(i, i, uint8_t(i));
    data.sheets.set(255, 767, 63);
    data.ramps[0] = {5, 6};
    data.ramps[7] = {1};
    const std::string text = write_spr(data);
    SprData back;
    std::string error;
    CHECK(parse(text, back, error));
    CHECK(back.sheets.px == data.sheets.px);
    CHECK(back.defined == data.defined);
    CHECK(back.ramps == data.ramps);
    CHECK(write_spr(back) == text);
    CHECK(text.find("sheet 1") == std::string::npos);
}

TEST(sprformat_reads_crlf_line_endings) {
    std::string text = sheet_text(0, 'C');
    std::string crlf;
    for (char c : text) crlf += c == '\n' ? std::string("\r\n") : std::string(1, c);
    SprData data;
    std::string error;
    CHECK(parse(crlf, data, error));
    CHECK_EQ(data.sheets.get(10, 10), 2);
}

TEST(sprformat_errors_name_the_line) {
    CHECK(fails_with("sheet 3\n", "x.spr:1: expected `sheet N` with N from 0 to 2"));
    std::string short_row = sheet_text(0, 'A');
    short_row.replace(short_row.find('\n', 9 + 4 * 257) - 1, 1, "");  // row 5 loses a pixel
    CHECK(fails_with(short_row, "x.spr:6: expected 256 pixel characters, found 255"));
    std::string bad_char = sheet_text(0, 'A');
    bad_char[8 + 9] = '#';
    CHECK(fails_with(bad_char, "x.spr:2: '#' at column 10"));
    CHECK(fails_with(sheet_text(0, 'A', 10), "has 10 rows"));
    CHECK(fails_with(sheet_text(0, 'A') + sheet_text(0, 'A'), "sheet 0 is defined twice"));
    CHECK(fails_with("ramp 8 1\n", "x.spr:1:"));
    CHECK(fails_with("ramp 0\n", "x.spr:1:"));
    CHECK(fails_with("ramp 0 64\n", "not a palette index"));
    CHECK(fails_with("ramp 0 1\nramp 0 2\n", "x.spr:2: ramp 0 is defined twice"));
    CHECK(fails_with("\n\nbogus\n", "x.spr:3: expected `sheet N` or `ramp N i i ...`"));
}

TEST(sprformat_empty_file_is_valid) {
    SprData data;
    std::string error;
    CHECK(parse("", data, error));
    CHECK(parse("\n  \n", data, error));
    CHECK(!data.defined[0]);
}
