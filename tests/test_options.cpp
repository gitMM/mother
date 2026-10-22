#include <initializer_list>
#include <string>
#include <vector>

#include "runner/options.h"
#include "test.h"

using namespace mother;

namespace {

ParseResult parse(std::initializer_list<const char*> args) {
    return parse_options(std::vector<std::string>(args.begin(), args.end()));
}

}  // namespace

TEST(options_run_headless_with_ticks) {
    const ParseResult r = parse({"run", "--headless", "--ticks", "3600"});
    CHECK(r.ok);
    CHECK(r.opts.headless);
    CHECK_EQ(r.opts.ticks, 3600);
    CHECK(r.opts.cart.empty());
}

TEST(options_run_cart_windowed) {
    const ParseResult r = parse({"run", "carts/knightfall"});
    CHECK(r.ok);
    CHECK_EQ(r.opts.cart, std::string("carts/knightfall"));
    CHECK(!r.opts.headless);
    CHECK_EQ(r.opts.ticks, -1);
}

TEST(options_run_windowed_with_tick_limit) {
    const ParseResult r = parse({"run", "--ticks", "120"});
    CHECK(r.ok);
    CHECK_EQ(r.opts.ticks, 120);
}

TEST(options_wav_needs_headless) {
    const ParseResult r = parse({"run", "c", "--headless", "--ticks", "60", "--wav", "out.wav"});
    CHECK(r.ok);
    CHECK_EQ(r.opts.wav, std::string("out.wav"));
    CHECK(!parse({"run", "c", "--wav", "out.wav"}).ok);
    CHECK(!parse({"run", "c", "--headless", "--ticks", "60", "--wav"}).ok);
}

TEST(options_headless_requires_ticks) {
    CHECK(!parse({"run", "--headless"}).ok);
}

TEST(options_rejects_bad_input) {
    CHECK(!parse({}).ok);
    CHECK(!parse({"play"}).ok);
    CHECK(!parse({"run", "--ticks"}).ok);
    CHECK(!parse({"run", "--ticks", "abc"}).ok);
    CHECK(!parse({"run", "--ticks", "-5"}).ok);
    CHECK(!parse({"run", "--ticks", "12x"}).ok);
    CHECK(!parse({"run", "--bogus"}).ok);
    CHECK(!parse({"run", "a", "b"}).ok);
}
