#include "assets/trk_cell.h"

#include <cstring>

#include "assets/text_lines.h"

namespace mother {

namespace {

constexpr const char* kNoteNames[12] = {"C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"};
constexpr char kHex[] = "0123456789ABCDEF";

int hex_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

// Two hex digits -> 0-255, ".." -> -1 (when allowed), anything else -> -2.
int parse_byte(std::string_view text, bool allow_none) {
    if (text.size() != 2) return -2;
    if (allow_none && text == "..") return -1;
    const int hi = hex_value(text[0]), lo = hex_value(text[1]);
    return hi < 0 || lo < 0 ? -2 : hi * 16 + lo;
}

std::string hex2(int v) {
    return {kHex[(v >> 4) & 15], kHex[v & 15]};
}

}  // namespace

bool parse_note(std::string_view text, int& note) {
    if (text == "---") return note = 0, true;
    if (text == "===") return note = kNoteOff, true;
    if (text.size() != 3 || text[2] < '0' || text[2] > '7') return false;
    for (int i = 0; i < 12; ++i) {
        if (text.substr(0, 2) == kNoteNames[i]) {
            note = (text[2] - '0') * 12 + i + 1;
            return true;
        }
    }
    return false;
}

bool parse_instrument_number(std::string_view text, int& n) {
    const int value = parse_byte(text, false);
    if (value < 0 || value >= kInstruments) return false;
    n = value;
    return true;
}

bool parse_fx(std::string_view text, char& fx, uint8_t& param) {
    if (text == "...") return fx = 0, param = 0, true;
    if (text.size() != 3 || std::strchr("PUDVT", text[0]) == nullptr || text[0] == 0) return false;
    const int value = parse_byte(text.substr(1), false);
    if (value < 0) return false;
    fx = text[0];
    param = uint8_t(value);
    return true;
}

bool parse_cell(std::string_view text, Cell& out, std::string& error) {
    const std::vector<std::string_view> words = split_words(text);
    if (words.size() != 4) {
        error = "expected a cell `NOTE INST VOL FX` like `C-4 01 40 ...`";
        return false;
    }
    Cell cell;
    int note = 0;
    if (!parse_note(words[0], note)) {
        error = "'" + std::string(words[0]) + "' is not a note (C-4, C#4 .. B-7, --- or ===)";
        return false;
    }
    cell.note = int8_t(note);
    const int inst = parse_byte(words[1], true);
    if (inst < -1 || inst >= kInstruments) {
        error = "'" + std::string(words[1]) + "' is not an instrument (00-3F or ..)";
        return false;
    }
    cell.inst = int8_t(inst);
    const int vol = parse_byte(words[2], true);
    if (vol < -1 || vol > kMaxVolume) {
        error = "'" + std::string(words[2]) + "' is not a volume (00-40 or ..)";
        return false;
    }
    cell.vol = int8_t(vol);
    if (!parse_fx(words[3], cell.fx, cell.param)) {
        error = "'" + std::string(words[3]) + "' is not an effect (Pxx Uxx Dxx Vxy Txx or ...)";
        return false;
    }
    out = cell;
    return true;
}

std::string format_note(int note) {
    if (note == 0) return "---";
    if (note == kNoteOff) return "===";
    return std::string(kNoteNames[(note - 1) % 12]) + char('0' + (note - 1) / 12);
}

std::string format_fx(char fx, uint8_t param) {
    return fx == 0 ? std::string("...") : fx + hex2(param);
}

std::string format_cell(const Cell& cell) {
    return format_note(cell.note) + " " + (cell.inst < 0 ? std::string("..") : hex2(cell.inst)) + " " +
           (cell.vol < 0 ? std::string("..") : hex2(cell.vol)) + " " + format_fx(cell.fx, cell.param);
}

}  // namespace mother
