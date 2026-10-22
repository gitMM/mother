#include <filesystem>
#include <memory>
#include <string>

#include "assets/spr_format.h"
#include "temp_cart.h"
#include "test.h"
#include "vm/cart_vm.h"

using namespace mother;

namespace {

std::unique_ptr<CartVM> open_cart(const std::filesystem::path& dir) {
    CartSource src;
    std::string error;
    if (!load_cart_source(dir, src, error)) {
        std::printf("  load failed: %s\n", error.c_str());
        return nullptr;
    }
    auto vm = std::make_unique<CartVM>();
    vm->open(std::move(src));
    return vm;
}

std::unique_ptr<CartVM> open_main(const std::string& name, const std::string& main_luau,
                                  const TempCart::Files& extra = {}) {
    TempCart::Files files = {{"cart.toml", kTestToml}, {"main.luau", main_luau}};
    files.insert(files.end(), extra.begin(), extra.end());
    TempCart cart(name, files);
    return open_cart(cart.dir());
}

bool has(const std::string& text, const char* part) {
    if (text.find(part) != std::string::npos) return true;
    std::printf("  \"%s\" not found in: %s\n", part, text.c_str());
    return false;
}

Outcome outcome(const CartVM& vm) {
    return vm.status().outcome;
}

void tick(CartVM& vm, const RawInput& raw, PadState& pad, uint64_t t) {
    pad = build_pad(raw, pad);
    vm.update(pad, t);
}

}  // namespace

TEST(cartvm_hello_draws_and_moves_with_input) {
    auto vm = open_cart(std::filesystem::path(MOTHER_TEST_CARTS) / "hello");
    CHECK(vm && outcome(*vm) == Outcome::Running);
    if (!vm) return;
    Framebuffer fb;
    vm->draw(fb);
    CHECK_EQ(fb.pget(160, 90), 22);  // the box at its start
    CHECK_EQ(fb.pget(180, 90), 17);  // background
    CHECK_EQ(fb.pget(40, 140), 12);  // filled circle

    RawInput right;
    right.right = true;
    PadState pad;
    for (uint64_t t = 0; t < 10; ++t) tick(*vm, right, pad, t);
    vm->draw(fb);
    CHECK_EQ(fb.pget(180, 90), 22);  // moved 2 px per tick
    CHECK_EQ(fb.pget(160, 85), 17);
    CHECK(outcome(*vm) == Outcome::Running);
}

TEST(cartvm_hello_reads_buttons_and_pointer) {
    auto vm = open_cart(std::filesystem::path(MOTHER_TEST_CARTS) / "hello");
    if (!vm) return;
    RawInput raw;
    raw.buttons[size_t(face_index(Button::A))] = true;
    raw.pointer = true;
    raw.pointer_x = 100;
    raw.pointer_y = 50;
    PadState pad;
    tick(*vm, raw, pad, 0);
    Framebuffer fb;
    vm->draw(fb);
    CHECK_EQ(fb.pget(160, 90), 9);   // flashing after A
    CHECK_EQ(fb.pget(103, 50), 22);  // pointer ring, button up
    raw.pointer_down = true;
    tick(*vm, raw, pad, 1);
    vm->draw(fb);
    CHECK_EQ(fb.pget(103, 50), 9);   // pointer ring, button down
}

TEST(cartvm_sandbox_cart_wins) {
    auto vm = open_cart(std::filesystem::path(MOTHER_TEST_CARTS) / "sandbox");
    CHECK(vm && outcome(*vm) == Outcome::Running);
    if (!vm) return;
    vm->update(PadState{}, 0);
    CHECK(outcome(*vm) == Outcome::Win);
}

TEST(cartvm_compile_errors_name_file_and_line) {
    auto vm = open_main("syntax", "local x = 1\nfunction cart.init(\n");
    CHECK(outcome(*vm) == Outcome::Error);
    CHECK(has(vm->status().message, "main.luau:"));
    auto bad_module = open_main("syntax_module", "local m = import(\"m\")\n" + lifecycle(""),
                                {{"m.luau", "return 1 +\n"}});
    CHECK(has(bad_module->status().message, "m.luau:"));
}

TEST(cartvm_runtime_errors_stop_the_cart) {
    auto vm = open_main("runtime", lifecycle("", "if tick() == 3 then error(\"boom\") end"));
    CHECK(outcome(*vm) == Outcome::Running);
    for (uint64_t t = 0; t < 5; ++t) vm->update(PadState{}, t);
    CHECK(outcome(*vm) == Outcome::Error);
    CHECK(has(vm->status().message, "main.luau:2: boom"));
    CHECK(has(vm->status().message, "stack traceback"));
}

TEST(cartvm_requires_every_lifecycle_function) {
    auto vm = open_main("no_hash", "function cart.init() end\nfunction cart.update() end\n"
                                   "function cart.draw() end\n");
    CHECK(outcome(*vm) == Outcome::Error);
    CHECK(has(vm->status().message, "cart.hash"));
}

TEST(cartvm_import_runs_each_module_once) {
    auto vm = open_main("import_once",
                        "local a, b = import(\"counter\"), import(\"counter\")\n" +
                            lifecycle("mother.finish(if a == b and a.loads == 1 then \"win\" else \"lose\")"),
                        {{"counter.luau", "local m = { loads = 0 }\nm.loads += 1\nreturn m\n"}});
    CHECK(outcome(*vm) == Outcome::Win);
}

TEST(cartvm_import_cycles_are_errors) {
    auto vm = open_main("import_cycle", "import(\"a\")\n" + lifecycle(""),
                        {{"a.luau", "return import(\"b\")"}, {"b.luau", "return import(\"a\")"}});
    CHECK(outcome(*vm) == Outcome::Error);
    CHECK(has(vm->status().message, "import cycle"));
}

TEST(cartvm_modules_have_their_own_globals) {
    auto vm = open_main("globals", "SHARED = 1\nlocal seen = import(\"m\")\n" +
                                       lifecycle("mother.finish(if seen == nil then \"win\" else \"lose\")"),
                        {{"m.luau", "return SHARED"}});
    CHECK(outcome(*vm) == Outcome::Win);
}

TEST(cartvm_graphics_only_in_cart_draw) {
    auto vm = open_main("draw_only", lifecycle("", "cls(0)"));
    vm->update(PadState{}, 0);
    CHECK(outcome(*vm) == Outcome::Error);
    CHECK(has(vm->status().message, "cls() can only be called from cart.draw()"));
}

TEST(cartvm_draw_api_reaches_the_framebuffer) {
    auto vm = open_main("draw_api", lifecycle("", "", R"(
        cls(1)
        camera(10, 0)
        pset(15, 5, 8)
        if pget(15, 5) ~= 8 then error("pget") end
        camera()
        pal(3, 4)
        rectfill(0, 10, 2, 2, 3)
        pal()
        clip(50, 50, 10, 10)
        rectfill(0, 0, 320, 180, 2)
        clip()
        line(0, 179, 3, 179, 5)
        circfill(300, 20, 2, 6)
    )"));
    Framebuffer fb;
    vm->draw(fb);
    CHECK(outcome(*vm) == Outcome::Running);
    CHECK_EQ(fb.pget(5, 5), 8);
    CHECK_EQ(fb.pget(0, 10), 4);
    CHECK_EQ(fb.pget(50, 50), 2);
    CHECK_EQ(fb.pget(59, 59), 2);
    CHECK_EQ(fb.pget(60, 60), 1);
    CHECK_EQ(fb.pget(3, 179), 5);
    CHECK_EQ(fb.pget(300, 20), 6);
}

TEST(cartvm_tick_counts_ticks_and_finish_stops) {
    auto vm = open_main("tick", lifecycle("if tick() ~= 0 then error(\"init tick\") end",
                                          "if tick() == 5 then mother.finish(\"win\") end"));
    CHECK(outcome(*vm) == Outcome::Running);
    for (uint64_t t = 0; t < 4; ++t) vm->update(PadState{}, t);
    CHECK(outcome(*vm) == Outcome::Running);
    vm->update(PadState{}, 4);
    CHECK(outcome(*vm) == Outcome::Win);
    CHECK_EQ(vm->ticks(), uint64_t(5));
}

TEST(cartvm_finish_takes_win_or_lose_only) {
    auto vm = open_main("finish_bad", lifecycle("mother.finish(\"draw\")"));
    CHECK(outcome(*vm) == Outcome::Error);
    CHECK(has(vm->status().message, "\"win\" or \"lose\""));
    auto lose = open_main("finish_lose", lifecycle("mother.finish(\"lose\")"));
    CHECK(outcome(*lose) == Outcome::Lose);
}

TEST(cartvm_heap_is_limited_to_16mb) {
    auto vm = open_main("heap", lifecycle("local s = string.rep(\"x\", 20 * 1024 * 1024)"));
    CHECK(outcome(*vm) == Outcome::Error);
    CHECK(has(vm->status().message, "16 MB"));
    auto fits = open_main("heap_fits", lifecycle("local s = string.rep(\"x\", 4 * 1024 * 1024)"));
    CHECK(outcome(*fits) == Outcome::Running);
}

TEST(cartvm_pow_is_deterministic) {
    // Constant-folded (2 ^ 10) and run-time (pow(2, 10)) results must agree.
    auto vm = open_main("pow", R"(
        local function pow(a, b) return a ^ b end
        local ok = 2 ^ 10 == 1024 and pow(2, 10) == 1024 and pow(2, -2) == 0.25
            and (-2) ^ 3 == -8 and pow(-2, 3) == -8 and pow(2, 0) == 1 and pow(3, 2) == 9
            and 2 ^ 0.5 == math.sqrt(2) and pow(2, 0.5) == math.sqrt(2)
            and math.isnan(2 ^ 1.5) and math.isnan(pow(2, 1.5))
        )" + lifecycle("mother.finish(if ok then \"win\" else \"lose\")"));
    CHECK(outcome(*vm) == Outcome::Win);
}

TEST(cartvm_sprite_map_and_text_api) {
    SprData art;
    art.defined[0] = true;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) art.sheets.set(8 + x, y, 7);  // sprite 1
    }
    art.ramps[2] = {1, 2, 3};
    const std::string map = "layer 0 2 1\n0001 0000\n" + flags_block(1, "03");
    auto vm = open_main("sprite_api", R"(
        local ok = mget(0, 0) == 1 and mget(1, 0) == 0 and mget(5, 5) == 0 and mget(0, 0, 1) == 0
            and fget(1) == 3 and fget(1, 0) == true and fget(1, 2) == false
        fset(1, 2, true)
        ok = ok and fget(1) == 7
        mset(1, 0, 1)
        mset(9, 9, 1)
        ok = ok and mget(1, 0) == 1
        local r = ramp(2)
        ok = ok and #r == 3 and r[1] == 1 and r[3] == 3 and #ramp(7) == 0
        ok = ok and not pcall(mset, 0, 0, 3072) and not pcall(fget, 1, 8) and not pcall(ramp, 8)
            and not pcall(mget, 0, 0, 2) and not pcall(fget, 3072)
        )" + lifecycle("", "if not ok then error(\"checks failed\") end", R"(
            cls(0)
            map(0, 0, 0, 0, 2, 1)
            spr(1, 20, 0)
            print("A", 40, 0, 9)
            sspr(8, 0, 8, 8, 60, 0, 16, 16)
            palt(7, true)
            spr(1, 80, 0)
            palt()
            if pcall(spr, 4096, 0, 0) then error("spr 4096") end
        )"),
                       {{"sprites.spr", write_spr(art)}, {"map.map", map}});
    vm->update(PadState{}, 0);
    CHECK(outcome(*vm) == Outcome::Running);
    Framebuffer fb;
    vm->draw(fb);
    CHECK(outcome(*vm) == Outcome::Running);
    if (outcome(*vm) != Outcome::Running) std::printf("  %s\n", vm->status().message.c_str());
    CHECK_EQ(fb.pget(0, 0), 7);   // map cell 0
    CHECK_EQ(fb.pget(8, 0), 7);   // map cell 1, set by mset
    CHECK_EQ(fb.pget(20, 0), 7);  // spr
    CHECK_EQ(fb.pget(40, 0), 9);  // print
    CHECK_EQ(fb.pget(75, 15), 7); // 2x sspr
    CHECK_EQ(fb.pget(80, 0), 0);  // palt made colour 7 transparent
}

TEST(cartvm_sprite_drawing_only_in_cart_draw) {
    auto vm = open_main("spr_draw_only", lifecycle("", "spr(0, 0, 0)"));
    vm->update(PadState{}, 0);
    CHECK(has(vm->status().message, "spr() can only be called from cart.draw()"));
}

TEST(cartvm_audio_api) {
    const std::string trk = "sfx 3 2\nC-5 .. .. ...\n\nsong 1 4 loop\n0\n";
    auto vm = open_main("audio_api", R"(
        local ok = beat() == nil
        local note, inst, vol, fx = song_get(0, 5, 2)
        ok = ok and note == 0 and inst == -1 and vol == -1 and fx == "..."
        song_set(0, 5, 2, 49, 1, 64, "V42")
        note, inst, vol, fx = song_get(0, 5, 2)
        ok = ok and note == 49 and inst == 1 and vol == 64 and fx == "V42"
        song_set(0, 5, 2, -1, -1, -1, nil)
        ok = ok and song_get(0, 5, 2) == -1
        ok = ok and not pcall(song_set, 0, 5, 2, 97, 0, 0, nil) and not pcall(song_set, 0, 5, 2, 1, 0, 0, "X00")
            and not pcall(song_get, 64, 0, 1) and not pcall(song_get, 0, 0, 7)
        ok = ok and not pcall(sfx, 0) and not pcall(sfx, 3, 7) and not pcall(sfx, 3, 0) and not pcall(music, 0)
            and not pcall(stop, 8) and not pcall(music, 1, -1)
        local reserved = select(2, pcall(sfx, 3, 8))
        ok = ok and string.find(reserved, "reserved") ~= nil
        sfx(3)
        sfx(3, 6)
        music(1)
        local bar, b, row = beat()
        ok = ok and bar == 0 and b == 0 and row == 0
        )" + lifecycle("", "if not ok then error(\"checks failed\") end", "if pcall(sfx, 3) then error(\"sfx in draw\") end"),
                       {{"audio.trk", trk}});
    CHECK(outcome(*vm) == Outcome::Running);
    vm->update(PadState{}, 0);
    CHECK(outcome(*vm) == Outcome::Running);
    if (outcome(*vm) != Outcome::Running) std::printf("  %s\n", vm->status().message.c_str());
    TickAudio audio;
    vm->take_audio(audio);
    int notes = 0;
    for (const AudioCommand& c : audio) notes += c.kind == AudioCommand::Kind::NoteOn ? 1 : 0;
    CHECK_EQ(notes, 2);  // the two sfx; song 1's pattern 0 is empty
    Framebuffer fb;
    vm->draw(fb);
    CHECK(outcome(*vm) == Outcome::Running);
}

TEST(cartvm_logs_budget_errors) {
    auto vm = open_main("budget", lifecycle("", "local x = 0 for i = 1, 30000000 do x += 1 end"));
    vm->update(PadState{}, 0);
    CHECK(outcome(*vm) == Outcome::Running);
    CHECK_EQ(vm->budget_errors(), 1);
}
