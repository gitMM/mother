#include <algorithm>
#include <string>

#include "runner/headless.h"
#include "runner/test_pattern.h"
#include "test.h"

using namespace mother;

namespace {

struct Counter : Program {
    uint64_t updates = 0, draws = 0, last_tick = 0;
    void update(const PadState&, uint64_t tick) override {
        ++updates;
        last_tick = tick;
    }
    void draw(Framebuffer&) override { ++draws; }
};

}  // namespace

TEST(headless_runs_exactly_the_requested_ticks) {
    Counter c;
    const RunResult r = run_headless(c, 3600);
    CHECK_EQ(std::string(r.result), std::string("timeout"));
    CHECK_EQ(r.ticks, uint64_t(3600));
    CHECK_EQ(c.updates, uint64_t(3600));
    CHECK_EQ(c.draws, uint64_t(3600));
    CHECK_EQ(c.last_tick, uint64_t(3599));
}

TEST(headless_3600_ticks_are_repeatable) {
    TestPattern a, b;
    const RunResult ra = run_headless(a, 3600);
    const RunResult rb = run_headless(b, 3600);
    CHECK_EQ(ra.ticks, rb.ticks);
    Framebuffer fa, fb;
    a.draw(fa);
    b.draw(fb);
    CHECK(std::equal(fa.data(), fa.data() + kScreenW * kScreenH, fb.data()));
}
