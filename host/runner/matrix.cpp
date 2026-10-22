// mother matrix (brief section 9, "Matrix test"): every export of every cart against every other
// cart. The table goes to --out: H for an export's home cart, ✗ where the host can't take it, ✓
// where it can. Each ✓ pair then runs headless for each fuzz seed (the same number for the cart's
// RNG) until the cart finishes or --ticks ticks pass, twice, and fails on:
//   a Luau error (a NaN position is one: MOTHERLIB raises it)
//   ticks over the CPU budget in both runs (the budget is wall time, so one run over it alone
//   is reported as a warning: the OS can pause any process for a few milliseconds)
//   an entity wholly outside the bounds for more than 60 ticks
//   a different state hash on any tick of the second run (the replay)
//   no progress towards the objective for 1,800 ticks while an imported solid is in the world
// Failures go to stderr, one line each, and make the exit code 1.

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "runner/commands.h"
#include "runner/input_source.h"
#include "runtime/hash.h"
#include "vm/cart_vm.h"

namespace mother {

namespace {

namespace fs = std::filesystem;

constexpr int kOutsideLimit = 60;
constexpr int kStallLimit = 1800;

struct Cart {
    std::string dir;   // folder name
    std::string title;
    CartSource source;
};

struct Run {
    std::string failure;  // empty when it passed
    uint64_t ticks = 0;
    uint64_t digest = 0;  // FNV-1a 64 over every tick's state hash
    uint64_t over = 0;    // the first tick over the CPU budget, 0 for none
};

Run run_pair(const Cart& host, const ImportSource& import, uint64_t seed, int64_t ticks) {
    Run out;
    CartVM vm;
    if (!vm.open(host.source, seed, {import})) {
        out.failure = "open: " + vm.status().message.substr(0, vm.status().message.find('\n'));
        return out;
    }
    FuzzSource input(seed);
    PadState pad;
    Framebuffer fb;
    TickAudio audio;
    Fnv64 digest;
    double best = -1;
    int stalled = 0;
    char where[64];
    for (int64_t t = 0; t < ticks && vm.status().outcome == Outcome::Running; ++t) {
        pad = input.next(pad);
        vm.update(pad, uint64_t(t));
        vm.take_audio(audio);
        if (vm.status().outcome == Outcome::Running) vm.draw(fb);
        std::snprintf(where, sizeof where, " (tick %" PRId64 ")", t + 1);
        out.ticks = uint64_t(t + 1);
        if (vm.status().outcome == Outcome::Error) {
            out.failure = "Luau error" + std::string(where) + ": " + vm.status().message.substr(0, vm.status().message.find('\n'));
            return out;
        }
        digest.u64(vm.state_hash());
        if (vm.budget_errors() > 0 && out.over == 0) out.over = uint64_t(t + 1);
        WorldAudit audit;
        if (!vm.audit(audit)) {
            out.failure = "audit failed" + std::string(where) + ": " + vm.status().message;
            return out;
        }
        if (audit.out_most > kOutsideLimit) {
            out.failure = "outside the bounds for over 60 ticks: " + audit.out_who + where;
            return out;
        }
        stalled = audit.solids && audit.progress <= best ? stalled + 1 : 0;
        if (audit.progress > best) best = audit.progress;
        if (stalled > kStallLimit) {
            out.failure = "no progress for 1800 ticks with an imported solid in the world" + std::string(where);
            return out;
        }
    }
    out.digest = digest.value();
    return out;
}

bool load_carts(const fs::path& dir, std::vector<Cart>& carts) {
    std::error_code ec;
    std::vector<fs::path> dirs;
    for (const fs::directory_entry& e : fs::directory_iterator(dir, ec)) {
        if (e.is_directory() && fs::is_regular_file(e.path() / "cart.toml")) dirs.push_back(e.path());
    }
    if (ec || dirs.empty()) {
        std::fprintf(stderr, "mother: matrix: no carts in %s\n", dir.string().c_str());
        return false;
    }
    std::sort(dirs.begin(), dirs.end());
    for (const fs::path& d : dirs) {
        Cart cart;
        std::string error;
        if (!load_cart_source(d, cart.source, error)) {
            std::fprintf(stderr, "mother: matrix: %s: %s\n", d.string().c_str(), error.c_str());
            return false;
        }
        cart.dir = d.filename().string();
        cart.title = cart.source.manifest.find("cart")->find("title")->string;
        carts.push_back(std::move(cart));
    }
    return true;
}

}  // namespace

int matrix_command(const Options& opts) {
    const fs::path dir(opts.cart);
    std::vector<Cart> carts;
    if (!load_carts(dir, carts)) return 1;

    std::string table = "# Import matrix\n\nWritten by `mother matrix`: H home cart, ✓ compatible, ✗ incompatible.\n\n";
    table += "| Export |";
    std::string rule = "| --- |";
    for (const Cart& c : carts) table += " " + c.title + " |", rule += " --- |";
    table += "\n" + rule + "\n";

    int failures = 0, pairs = 0;
    for (const Cart& source : carts) {
        const TomlValue* exports = source.source.manifest.find("exports");
        for (size_t i = 0; exports && i < exports->items.size(); ++i) {
            const std::string id = exports->items[i].find("id")->string;
            ImportSource import;
            std::string error;
            if (!load_import(dir / source.dir, source.dir + ":" + id, import, error)) {
                std::fprintf(stderr, "mother: matrix: %s\n", error.c_str());
                return 1;
            }
            table += "| " + id + " |";
            for (const Cart& host : carts) {
                if (&host == &source) {
                    table += " H |";
                    continue;
                }
                CartVM probe;
                if (!probe.open(host.source, 1, {import}) && !probe.incompatible().empty()) {
                    table += " ✗ |";
                    continue;
                }
                table += " ✓ |";
                ++pairs;
                for (int seed = 1; seed <= opts.seeds; ++seed) {
                    const Run first = run_pair(host, import, uint64_t(seed), opts.ticks);
                    std::string failure = first.failure;
                    if (failure.empty()) {
                        const Run again = run_pair(host, import, uint64_t(seed), opts.ticks);
                        if (!again.failure.empty()) {
                            failure = "replay: " + again.failure;
                        } else if (again.digest != first.digest || again.ticks != first.ticks) {
                            failure = "the replay's state hashes differ";
                        } else if (first.over > 0 && again.over > 0) {
                            failure = "over the CPU budget in both runs (ticks " + std::to_string(first.over) + " and " +
                                      std::to_string(again.over) + ")";
                        } else if (first.over > 0 || again.over > 0) {
                            std::fprintf(stderr, "warning: %s:%s in %s, seed %d: one run went over the CPU budget at tick %llu\n",
                                         source.dir.c_str(), id.c_str(), host.dir.c_str(), seed,
                                         static_cast<unsigned long long>(first.over + again.over));
                        }
                    }
                    if (!failure.empty()) {
                        ++failures;
                        std::fprintf(stderr, "FAIL %s:%s in %s, seed %d: %s\n", source.dir.c_str(), id.c_str(),
                                     host.dir.c_str(), seed, failure.c_str());
                    }
                }
            }
            table += "\n";
        }
    }

    std::ofstream out(fs::path(opts.out), std::ios::binary | std::ios::trunc);
    if (!out.write(table.data(), std::streamsize(table.size()))) {
        std::fprintf(stderr, "mother: could not write %s\n", opts.out.c_str());
        return 1;
    }
    std::printf("matrix: %d compatible pairs, %d seeds of up to %" PRId64 " ticks each, %d failures; table in %s\n",
                pairs, opts.seeds, opts.ticks, failures, opts.out.c_str());
    return failures > 0 ? 1 : 0;
}

}  // namespace mother
