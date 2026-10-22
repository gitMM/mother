#include <algorithm>
#include <string>
#include <vector>

#include "runner/headless.h"
#include "runner/test_pattern.h"
#include "test.h"

using namespace mother;

namespace {

struct Counter : Program {
    uint64_t updates = 0, draws = 0, last_tick = 0;
    std::vector<PadState> pads;
    void update(const PadState& pad, uint64_t tick) override {
        ++updates;
        last_tick = tick;
        pads.push_back(pad);
    }
    void draw(Framebuffer&) override { ++draws; }
    uint64_t state_hash() const override { return updates * 10; }
};

}  // namespace

TEST(headless_runs_exactly_the_requested_ticks) {
    Counter c;
    const RunResult r = run_headless(c, {3600});
    CHECK_EQ(std::string(r.result), std::string("timeout"));
    CHECK_EQ(r.ticks, uint64_t(3600));
    CHECK_EQ(c.updates, uint64_t(3600));
    CHECK_EQ(c.draws, uint64_t(3600));
    CHECK_EQ(c.last_tick, uint64_t(3599));
    CHECK_EQ(r.state_hash, uint64_t(36000));
}

TEST(headless_3600_ticks_are_repeatable) {
    TestPattern a, b;
    const RunResult ra = run_headless(a, {3600});
    const RunResult rb = run_headless(b, {3600});
    CHECK_EQ(ra.ticks, rb.ticks);
    CHECK_EQ(ra.audio_hash, rb.audio_hash);
    Framebuffer fa, fb;
    a.draw(fa);
    b.draw(fb);
    CHECK(std::equal(fa.data(), fa.data() + kScreenW * kScreenH, fb.data()));
}

TEST(headless_feeds_the_input_source_and_reports_each_tick) {
    Counter c;
    FuzzSource fuzz(7);
    std::vector<uint64_t> ticks, hashes;
    HeadlessRun run{300, &fuzz};
    run.observe = [&](uint64_t tick, const PadState&, uint64_t hash) {
        ticks.push_back(tick);
        hashes.push_back(hash);
    };
    run_headless(c, run);
    CHECK_EQ(ticks.size(), size_t(300));
    CHECK_EQ(ticks.front(), uint64_t(1));
    CHECK_EQ(hashes.back(), uint64_t(3000));
    int moving = 0;
    for (const PadState& p : c.pads) moving += p.stick_x != 0 || p.stick_y != 0 ? 1 : 0;
    CHECK(moving > 50);  // the fuzzer moved the stick
}
