#include <string>

#include "runner/input_source.h"
#include "runner/trace.h"
#include "test.h"

using namespace mother;

namespace {

Trace sample() {
    Trace t;
    t.cart = "dice";
    t.seed = 1234567890123ull;
    TraceTick a;
    a.pad.held = button_bit(Button::A) | button_bit(Button::Left);
    a.pad.stick_x = -127;
    a.hash = 0x0123456789ABCDEFull;
    TraceTick b = a;
    b.pad.pointer = true;
    b.pad.pointer_x = 319;
    b.pad.pointer_y = 7;
    b.pad.pointer_down = true;
    b.hash = 0xFFFFFFFFFFFFFFFFull;
    TraceTick c;
    t.ticks = {a, b, c};
    return t;
}

bool fails_with(const std::string& text, const char* part) {
    Trace t;
    std::string error;
    if (parse_trace(text, "x.trace", t, error)) {
        std::printf("  parsed, expected an error containing \"%s\"\n", part);
        return false;
    }
    if (error.find(part) != std::string::npos) return true;
    std::printf("  \"%s\" not found in: %s\n", part, error.c_str());
    return false;
}

}  // namespace

TEST(trace_write_then_parse_round_trips) {
    const Trace t = sample();
    const std::string text = write_trace(t);
    CHECK(text.find("011 -127 0 - 0123456789abcdef\n") != std::string::npos);
    CHECK(text.find("011 -127 0 319,7,1 ffffffffffffffff\n") != std::string::npos);
    Trace back;
    std::string error;
    CHECK(parse_trace(text, "x.trace", back, error));
    CHECK_EQ(back.cart, std::string("dice"));
    CHECK_EQ(back.seed, t.seed);
    CHECK_EQ(back.ticks.size(), size_t(3));
    CHECK_EQ(back.ticks[1].pad.pointer_x, 319);
    CHECK(back.ticks[1].pad.pointer_down);
    CHECK(!back.ticks[2].pad.pointer);
    CHECK(write_trace(back) == text);
}

TEST(trace_errors_name_the_line) {
    const std::string head = "mother-trace 1\ncart c\nseed 0\nticks 1\n";
    CHECK(fails_with("hello\n", "x.trace:1: not a MOTHER trace"));
    CHECK(fails_with("mother-trace 1\ncart c\nseed x\nticks 0\n", "x.trace:3: expected `seed N`"));
    CHECK(fails_with(head, "expected 1 tick lines, found 0"));
    CHECK(fails_with(head + "800 0 0 - 0000000000000000\n", "x.trace:5:"));  // bit 11 is not a button
    CHECK(fails_with(head + "000 128 0 - 0000000000000000\n", "x.trace:5:"));
    CHECK(fails_with(head + "000 0 0 1,2 0000000000000000\n", "x.trace:5:"));
    CHECK(fails_with(head + "000 0 0 - 00000000\n", "x.trace:5:"));
}

TEST(trace_source_replays_pads_and_derives_presses) {
    const Trace t = sample();
    TraceSource source(t);
    PadState p1 = source.next(PadState{});
    CHECK(p1.btnp(Button::A));
    PadState p2 = source.next(p1);
    CHECK(p2.btn(Button::A) && !p2.btnp(Button::A));
    CHECK(p2.pointer && p2.pointer_x == 319);
    PadState p3 = source.next(p2);
    CHECK_EQ(p3.held, 0);
    PadState p4 = source.next(p3);  // past the end: neutral
    CHECK_EQ(p4.held, 0);
}
