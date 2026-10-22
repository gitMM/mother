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
    CHECK(!r.opts.has_seed);
    CHECK(r.opts.input == InputKind::Live);
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

TEST(options_rogue_defaults_to_the_carts_folder) {
    ParseResult r = parse({"rogue"});
    CHECK(r.ok);
    CHECK_EQ(r.opts.cart, std::string("carts"));
    CHECK(!r.opts.headless);
    r = parse({"rogue", "games", "--headless", "--ticks", "600", "--input", "fuzz:2", "--seed", "4"});
    CHECK(r.ok);
    CHECK_EQ(r.opts.cart, std::string("games"));
    CHECK(r.opts.input == InputKind::Fuzz);
    CHECK_EQ(r.opts.seed, uint64_t(4));
    CHECK(!parse({"rogue", "a", "b"}).ok);
    CHECK(!parse({"rogue", "--input", "trace:x.trace"}).ok);
    CHECK(!parse({"rogue", "--headless"}).ok);
}

TEST(options_seed_and_inputs) {
    ParseResult r = parse({"run", "c", "--headless", "--ticks", "18000", "--input", "fuzz:42", "--seed", "7"});
    CHECK(r.ok);
    CHECK(r.opts.input == InputKind::Fuzz);
    CHECK_EQ(r.opts.fuzz_seed, uint64_t(42));
    CHECK(r.opts.has_seed);
    CHECK_EQ(r.opts.seed, uint64_t(7));
    r = parse({"run", "c", "--headless", "--input", "trace:a.trace"});  // the trace sets the length
    CHECK(r.ok);
    CHECK(r.opts.input == InputKind::Trace);
    CHECK_EQ(r.opts.input_trace, std::string("a.trace"));
    CHECK(!parse({"run", "c", "--input", "trace:a.trace", "--seed", "3"}).ok);
    CHECK(!parse({"run", "c", "--input", "fuzz:x"}).ok);
    CHECK(!parse({"run", "c", "--input", "keyboard"}).ok);
    CHECK(!parse({"run", "c", "--seed", "-1"}).ok);
}

TEST(options_record_and_replay) {
    ParseResult r = parse({"record", "carts/rally", "out.trace"});
    CHECK(r.ok);
    CHECK_EQ(r.opts.command, std::string("record"));
    CHECK_EQ(r.opts.cart, std::string("carts/rally"));
    CHECK_EQ(r.opts.trace, std::string("out.trace"));
    CHECK(parse({"record", "c", "t", "--headless", "--ticks", "60", "--input", "fuzz:1"}).ok);
    CHECK(!parse({"record", "c", "t", "--input", "trace:x"}).ok);
    CHECK(!parse({"record", "c"}).ok);
    r = parse({"replay", "carts/rally", "out.trace", "--hash"});
    CHECK(r.ok);
    CHECK(r.opts.headless);
    CHECK(r.opts.print_hashes);
    CHECK(!parse({"replay", "c", "t", "--ticks", "5"}).ok);
    CHECK(!parse({"replay", "c", "t", "--seed", "5"}).ok);
    CHECK(!parse({"run", "c", "--hash"}).ok);
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
