#include "runner/clock.h"
#include "test.h"

using namespace mother;

TEST(clock_zero_elapsed_runs_nothing) {
    TickClock c;
    CHECK_EQ(c.advance(0), 0);
}

TEST(clock_runs_60_ticks_per_second_at_60_fps) {
    TickClock c;
    int total = 0;
    for (int i = 0; i < 60; ++i) {
        const int due = c.advance(16'666'667);
        CHECK_EQ(due, 1);
        total += due;
    }
    CHECK_EQ(total, 60);
}

TEST(clock_runs_exactly_60_ticks_in_uneven_frames) {
    TickClock c;
    int total = 0;
    for (int i = 0; i < 50; ++i) total += c.advance(20'000'000);  // 1.2 ticks each
    CHECK_EQ(total, 60);
    CHECK_EQ(c.advance(0), 0);
}

TEST(clock_caps_at_4_ticks_and_drops_time) {
    TickClock c;
    CHECK_EQ(c.advance(1'000'000'000), 4);
    CHECK_EQ(c.advance(0), 0);
}

TEST(clock_keeps_subtick_remainder_after_cap) {
    TickClock c;
    CHECK_EQ(c.advance(1'000'000'000 + 8'333'333), 4);  // a stall plus half a tick
    CHECK_EQ(c.advance(8'333'333), 0);
    CHECK_EQ(c.advance(1), 1);
}
