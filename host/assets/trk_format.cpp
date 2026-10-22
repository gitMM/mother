#include "assets/trk_format.h"

#include <set>
#include <utility>

#include "assets/text_lines.h"
#include "assets/trk_cell.h"

namespace mother {

namespace {

struct Line {
    size_t number;  // 1-based
    std::string_view text;
    std::vector<std::string_view> words;
};

bool is_header(const Line& line) {
    if (line.words.empty()) return false;
    const std::string_view w = line.words[0];
    return w == "instrument" || w == "sfx" || w == "pattern" || w == "song" || w == "wavetable" || w == "fx";
}

bool in_range(std::string_view word, int lo, int hi, int& out) {
    return parse_int(word, out) && out >= lo && out <= hi;
}

class Parser {
public:
    Parser(std::string_view text, const std::string& filename) : filename_(filename) {
        size_t number = 0;
        for (std::string_view text_line : split_lines(text)) {
            ++number;
            const std::vector<std::string_view> words = split_words(text_line);
            if (!words.empty() && words[0].front() == '#') continue;
            lines_.push_back({number, text_line, words});
        }
    }

    bool run(TrackerData& out, std::string& error) {
        while (at_ < lines_.size()) {
            const Line& line = lines_[at_];
            if (line.words.empty()) {
                ++at_;
                continue;
            }
            const std::string_view kind = line.words[0];
            const bool ok = kind == "instrument" ? instrument()
                            : kind == "sfx"      ? sfx()
                            : kind == "pattern"  ? pattern()
                            : kind == "song"     ? song()
                            : kind == "wavetable" ? wavetable()
                            : kind == "fx"       ? fx()
                                                 : fail(line, "expected a block: instrument, sfx, pattern, song, wavetable or fx");
            if (!ok) {
                error = error_;
                return false;
            }
        }
        out = std::move(data_);
        return true;
    }

private:
    bool fail(const Line& line, const std::string& problem) {
        error_ = filename_ + ":" + std::to_string(line.number) + ": " + problem;
        return false;
    }

    bool once(const Line& line, const std::string& what) {
        if (seen_.insert(what).second) return true;
        return fail(line, what + " is defined twice");
    }

    // Lines of a block body: from the line after the header up to a blank line or header.
    std::vector<const Line*> body() {
        std::vector<const Line*> out;
        ++at_;
        while (at_ < lines_.size() && !lines_[at_].words.empty() && !is_header(lines_[at_])) out.push_back(&lines_[at_++]);
        return out;
    }

    // The single line after a header.
    const Line* next_line(const Line& header, const std::string& what) {
        ++at_;
        if (at_ >= lines_.size() || lines_[at_].words.empty() || is_header(lines_[at_])) {
            fail(header, what + " needs its data on the next line");
            return nullptr;
        }
        return &lines_[at_++];
    }

    bool instrument();
    bool sfx();
    bool pattern();
    bool song();
    bool wavetable();
    bool fx();

    std::string filename_;
    std::vector<Line> lines_;
    size_t at_ = 0;
    TrackerData data_;
    std::set<std::string> seen_;
    std::string error_;
};

bool Parser::instrument() {
    const Line& header = lines_[at_];
    int n = -1;
    if (header.words.size() != 2 || !parse_instrument_number(header.words[1], n)) {
        return fail(header, "expected `instrument XX` with XX two hex digits 00-3F");
    }
    if (!once(header, "instrument " + std::string(header.words[1]))) return false;
    VoiceParams& p = data_.instruments[size_t(n)];
    std::set<std::string_view> keys;
    for (const Line* line : body()) {
        const std::vector<std::string_view>& w = line->words;
        if (!keys.insert(w[0]).second) return fail(*line, "`" + std::string(w[0]) + "` is set twice");
        int a = 0, b = 0, c = 0, d = 0;
        if (w[0] == "wave") {
            if (w.size() == 2 && w[1] == "square12") p.wave = Wave::Square12;
            else if (w.size() == 2 && w[1] == "square25") p.wave = Wave::Square25;
            else if (w.size() == 2 && w[1] == "square50") p.wave = Wave::Square50;
            else if (w.size() == 2 && w[1] == "saw") p.wave = Wave::Saw;
            else if (w.size() == 2 && w[1] == "triangle") p.wave = Wave::Triangle;
            else if (w.size() == 2 && w[1] == "noise") p.wave = Wave::Noise;
            else if (w.size() == 3 && w[1] == "table" && in_range(w[2], 0, kWavetables - 1, a)) p.wave = Wave::Table, p.table = uint8_t(a);
            else return fail(*line, "expected `wave square12|square25|square50|saw|triangle|noise|table N`");
        } else if (w[0] == "adsr") {
            if (w.size() != 5 || !in_range(w[1], 0, 255, a) || !in_range(w[2], 0, 255, b) ||
                !in_range(w[3], 0, kMaxVolume, c) || !in_range(w[4], 0, 255, d)) {
                return fail(*line, "expected `adsr A D S R` with A, D, R 0-255 ticks and S 0-64");
            }
            p.attack = uint8_t(a), p.decay = uint8_t(b), p.sustain = uint8_t(c), p.release = uint8_t(d);
        } else if (w[0] == "volume") {
            if (w.size() != 2 || !in_range(w[1], 0, kMaxVolume, a)) return fail(*line, "expected `volume V` with V 0-64");
            p.volume = uint8_t(a);
        } else if (w[0] == "pan") {
            if (w.size() != 2 || !in_range(w[1], 0, 255, a)) return fail(*line, "expected `pan P` with P 0-255");
            p.pan = uint8_t(a);
        } else if (w[0] == "slide") {
            if (w.size() != 2 || !in_range(w[1], -128, 127, a)) return fail(*line, "expected `slide S` with S -128..127");
            p.slide = int8_t(a);
        } else if (w[0] == "vibrato") {
            if (w.size() != 3 || !in_range(w[1], 0, 255, a) || !in_range(w[2], 0, 255, b)) {
                return fail(*line, "expected `vibrato SPEED DEPTH`, both 0-255");
            }
            p.vibrato_speed = uint8_t(a), p.vibrato_depth = uint8_t(b);
        } else {
            return fail(*line, "unknown instrument key `" + std::string(w[0]) + "`");
        }
    }
    return true;
}

bool Parser::sfx() {
    const Line& header = lines_[at_];
    int n = -1, speed = 0;
    if (header.words.size() != 3 || !in_range(header.words[1], 0, kSfxCount - 1, n) ||
        !in_range(header.words[2], 1, 255, speed)) {
        return fail(header, "expected `sfx N SPEED` with N 0-63 and SPEED 1-255");
    }
    if (!once(header, "sfx " + std::to_string(n))) return false;
    Sfx& s = data_.sfx[size_t(n)];
    s.speed = speed;
    for (const Line* line : body()) {
        Cell cell;
        std::string err;
        if (!parse_cell(line->text, cell, err)) return fail(*line, err);
        if (s.rows.size() == size_t(kSfxRows)) return fail(*line, "an sfx has at most 64 rows");
        s.rows.push_back(cell);
    }
    if (s.rows.empty()) return fail(header, "sfx " + std::to_string(n) + " has no rows");
    return true;
}

bool Parser::pattern() {
    const Line& header = lines_[at_];
    int n = -1;
    if (header.words.size() != 2 || !in_range(header.words[1], 0, kPatterns - 1, n)) {
        return fail(header, "expected `pattern N` with N 0-63");
    }
    if (!once(header, "pattern " + std::to_string(n))) return false;
    const std::vector<const Line*> rows = body();
    if (rows.size() != size_t(kPatternRows)) {
        return fail(header, "pattern " + std::to_string(n) + " has " + std::to_string(rows.size()) +
                                " rows; it needs 64, with no blank lines");
    }
    for (int r = 0; r < kPatternRows; ++r) {
        const Line& line = *rows[size_t(r)];
        std::vector<std::string_view> cells;
        size_t start = 0;
        while (true) {
            const size_t bar = line.text.find('|', start);
            cells.push_back(line.text.substr(start, bar == std::string_view::npos ? std::string_view::npos : bar - start));
            if (bar == std::string_view::npos) break;
            start = bar + 1;
        }
        if (cells.size() != size_t(kChannels)) {
            return fail(line, "expected 6 cells separated by |, found " + std::to_string(cells.size()));
        }
        for (int c = 0; c < kChannels; ++c) {
            std::string err;
            if (!parse_cell(cells[size_t(c)], data_.patterns[size_t(n)][size_t(r)][size_t(c)], err)) {
                return fail(line, "channel " + std::to_string(c + 1) + ": " + err);
            }
        }
    }
    return true;
}

bool Parser::song() {
    const Line& header = lines_[at_];
    int n = -1, tempo = 0;
    if (header.words.size() != 4 || !in_range(header.words[1], 0, kSongs - 1, n) ||
        !in_range(header.words[2], 1, 255, tempo) || (header.words[3] != "loop" && header.words[3] != "once")) {
        return fail(header, "expected `song N TEMPO loop|once` with N 0-7 and TEMPO 1-255");
    }
    if (!once(header, "song " + std::to_string(n))) return false;
    const Line* line = next_line(header, "song " + std::to_string(n));
    if (!line) return false;
    if (line->words.size() > size_t(kSongLength)) return fail(*line, "a song has at most 64 patterns");
    Song& s = data_.songs[size_t(n)];
    s.tempo = tempo;
    s.loop = header.words[3] == "loop";
    for (std::string_view word : line->words) {
        int p = -1;
        if (!in_range(word, 0, kPatterns - 1, p)) return fail(*line, "'" + std::string(word) + "' is not a pattern 0-63");
        s.order.push_back(uint8_t(p));
    }
    return true;
}

bool Parser::wavetable() {
    const Line& header = lines_[at_];
    int n = -1;
    if (header.words.size() != 2 || !in_range(header.words[1], 0, kWavetables - 1, n)) {
        return fail(header, "expected `wavetable N` with N 0-31");
    }
    if (!once(header, "wavetable " + std::to_string(n))) return false;
    const Line* line = next_line(header, "wavetable " + std::to_string(n));
    if (!line) return false;
    if (line->words.size() != size_t(kWavetableSize)) {
        return fail(*line, "expected 32 samples, found " + std::to_string(line->words.size()));
    }
    for (int i = 0; i < kWavetableSize; ++i) {
        int v = 0;
        if (!in_range(line->words[size_t(i)], -128, 127, v)) {
            return fail(*line, "'" + std::string(line->words[size_t(i)]) + "' is not a sample -128..127");
        }
        data_.setup.wavetables[size_t(n)][size_t(i)] = int8_t(v);
    }
    return true;
}

bool Parser::fx() {
    const Line& header = lines_[at_];
    if (header.words.size() != 1) return fail(header, "expected `fx` alone on its line");
    if (!once(header, "fx")) return false;
    GlobalFx& f = data_.setup.fx;
    std::set<std::string_view> keys;
    for (const Line* line : body()) {
        const std::vector<std::string_view>& w = line->words;
        if (!keys.insert(w[0]).second) return fail(*line, "`" + std::string(w[0]) + "` is set twice");
        if (w[0] == "delay") {
            if (w.size() != 4 || !in_range(w[1], 0, 1000, f.delay_ms) || !in_range(w[2], 0, 64, f.delay_feedback) ||
                !in_range(w[3], 0, 64, f.delay_mix)) {
                return fail(*line, "expected `delay MS FEEDBACK MIX` with MS 0-1000 and the others 0-64");
            }
        } else if (w[0] == "lowpass") {
            if (w.size() != 2 || !in_range(w[1], 0, 255, f.lowpass)) return fail(*line, "expected `lowpass CUTOFF` with CUTOFF 0-255");
        } else if (w[0] == "bitcrush") {
            if (w.size() != 3 || !in_range(w[1], 1, 16, f.crush_bits) || !in_range(w[2], 1, 16, f.crush_rate)) {
                return fail(*line, "expected `bitcrush BITS RATE`, both 1-16");
            }
        } else {
            return fail(*line, "unknown fx key `" + std::string(w[0]) + "`");
        }
    }
    return true;
}

}  // namespace

bool parse_trk(std::string_view text, const std::string& filename, TrackerData& out, std::string& error) {
    return Parser(text, filename).run(out, error);
}

}  // namespace mother
