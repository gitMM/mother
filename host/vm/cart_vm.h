#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <set>
#include <string>

#include "audio/sequencer.h"
#include "gfx/draw.h"
#include "runtime/rng.h"
#include "runner/program.h"
#include "vm/cart_source.h"
#include "vm/motherlib.h"
#include "vm/sandbox.h"

struct lua_State;

namespace mother {

// One cart running in its own sandboxed Luau VM (brief section 4). Runs the cart's lifecycle
// functions and holds the state the cart API bindings read and write.
class CartVM : public Program {
public:
    static constexpr std::chrono::milliseconds kTickBudget{8};

    CartVM() = default;
    ~CartVM() override;
    CartVM(const CartVM&) = delete;
    CartVM& operator=(const CartVM&) = delete;

    // Seeds the RNG, loads MOTHERLIB and creates the world from cart.toml, runs main.luau,
    // checks that it defined cart.init, cart.update, cart.draw and cart.hash as functions, then
    // calls cart.init. On failure status() holds the error and it returns false.
    bool open(CartSource source, uint64_t seed = 0);

    // One tick: cart.update, world:step(), the audio sequencer, then the state hash.
    void update(const PadState& pad, uint64_t tick) override;
    void draw(Framebuffer& fb) override;
    const Status& status() const override { return status_; }
    const AudioSetup& audio_setup() const override { return source_.tracker.setup; }
    void take_audio(TickAudio& out) override;
    // FNV-1a 64 over the tick count, the RNG state, the tilemap hash, the world's state and
    // cart.hash()'s value.
    uint64_t state_hash() const override { return state_hash_; }
    int budget_errors() const override { return budget_errors_; }

    // For the API bindings.
    static CartVM& of(lua_State* L);
    Painter& painter_in_draw(lua_State* L, const char* fn);  // errors outside cart.draw()
    bool drawing() const { return painter_.drawing(); }
    const PadState& pad() const { return pad_; }
    TileMap& tilemap() { return source_.map; }
    const Ramps& ramps() const { return source_.art.ramps; }
    TrackerData& tracker() { return source_.tracker; }
    Sequencer& sequencer() { return sequencer_; }
    Rng& rng() { return rng_; }
    Motherlib& lib() { return lib_; }
    uint64_t ticks() const { return ticks_; }  // ticks started; 0 during cart.init
    void finish(Outcome outcome);
    void log(const std::string& line);
    int import_module(lua_State* L, const char* name);  // pushes the module's value
    void run_main(lua_State* L);                         // runs main.luau, from open()
    void open_lib(lua_State* L);                         // loads MOTHERLIB, from open()
    void check_budget();                                 // from the interrupt callback

private:
    void run_module(lua_State* L, const std::string& name);  // pushes its first result
    bool call(const char* fn, int nresults = 0);
    bool protected_call(int nargs, int nresults = 0);
    void update_hash();
    void fail(std::string message);
    void report_budget();

    Heap heap_;
    lua_State* L_ = nullptr;
    int api_ref_ = 0;
    CartSource source_;
    Sequencer sequencer_{source_.tracker};
    TickAudio audio_;  // this tick's audio commands
    Rng rng_;
    Motherlib lib_;
    uint64_t state_hash_ = 0;
    Painter painter_;
    PadState pad_;
    uint64_t ticks_ = 0;
    Status status_;
    std::map<std::string, int> imported_;  // module name -> registry ref of its value
    std::set<std::string> importing_;      // modules being loaded, for cycle errors
    std::chrono::steady_clock::time_point tick_start_;
    bool in_update_ = false;
    bool budget_logged_ = false;
    uint32_t interrupts_ = 0;
    int budget_errors_ = 0;
};

}  // namespace mother
