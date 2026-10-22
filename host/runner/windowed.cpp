#include "runner/windowed.h"

#include <cstdio>
#include <vector>

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED  // main() is ours; SDL_main.h only declares SDL_SetMainReady
#include <SDL3/SDL_main.h>

#include "audio/audio_out.h"
#include "gfx/present.h"
#include "input/keymap.h"
#include "runner/clock.h"

namespace mother {

namespace {

constexpr int kStartScale = 3;

struct Video {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    Viewport viewport = fit_viewport(kScreenW * kStartScale, kScreenH * kStartScale);

    ~Video() {
        if (texture) SDL_DestroyTexture(texture);
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
    }
};

bool open_video(Video& v) {
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (!SDL_CreateWindowAndRenderer("MOTHER", kScreenW * kStartScale, kScreenH * kStartScale, flags,
                                     &v.window, &v.renderer)) {
        return false;
    }
    SDL_SetRenderVSync(v.renderer, 1);
    v.texture = SDL_CreateTexture(v.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                  kScreenW, kScreenH);
    if (!v.texture) return false;
    SDL_SetTextureScaleMode(v.texture, SDL_SCALEMODE_NEAREST);
    return true;
}

void present(Video& v, const Framebuffer& fb, std::vector<uint32_t>& argb) {
    to_argb(fb, argb.data());
    SDL_UpdateTexture(v.texture, nullptr, argb.data(), kScreenW * int(sizeof(uint32_t)));

    int out_w = 0, out_h = 0;
    SDL_GetCurrentRenderOutputSize(v.renderer, &out_w, &out_h);
    v.viewport = fit_viewport(out_w, out_h);
    const SDL_FRect dst{float(v.viewport.x), float(v.viewport.y), float(v.viewport.w),
                        float(v.viewport.h)};

    SDL_SetRenderDrawColor(v.renderer, 0, 0, 0, 255);
    SDL_RenderClear(v.renderer);
    SDL_RenderTexture(v.renderer, v.texture, nullptr, &dst);
    SDL_RenderPresent(v.renderer);
}

// Adds the mouse to raw input while it is over the screen image.
void read_pointer(const Video& v, RawInput& raw) {
    if (SDL_GetMouseFocus() != v.window) return;
    float wx = 0, wy = 0, ox = 0, oy = 0;
    const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&wx, &wy);
    if (!SDL_RenderCoordinatesFromWindow(v.renderer, wx, wy, &ox, &oy)) return;
    int sx = 0, sy = 0;
    if (!output_to_screen(v.viewport, ox, oy, sx, sy)) return;
    raw.pointer = true;
    raw.pointer_x = int16_t(sx);
    raw.pointer_y = int16_t(sy);
    raw.pointer_down = (buttons & SDL_BUTTON_LMASK) != 0;
}

// Keeps the first connected gamepad open; returns false when the window should close.
bool pump_events(SDL_Gamepad*& gamepad) {
    SDL_Event e;
    bool running = true;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            running = false;
        } else if (e.type == SDL_EVENT_GAMEPAD_ADDED && !gamepad) {
            gamepad = SDL_OpenGamepad(e.gdevice.which);
        } else if (e.type == SDL_EVENT_GAMEPAD_REMOVED && gamepad &&
                   SDL_GetGamepadID(gamepad) == e.gdevice.which) {
            SDL_CloseGamepad(gamepad);
            gamepad = nullptr;
        }
    }
    return running;
}

RunResult run_loop(Program& prog, const WindowedRun& run, Video& video) {
    Framebuffer fb;
    std::vector<uint32_t> argb(size_t(kScreenW * kScreenH));
    SDL_Gamepad* gamepad = nullptr;
    AudioOut audio(prog.audio_setup());
    TickAudio commands;
    TickClock clock;
    PadState pad;
    uint64_t ticks = 0;
    uint64_t last = SDL_GetTicksNS();
    auto running = [&] {
        return prog.status().outcome == Outcome::Running && (run.max_ticks < 0 || ticks < uint64_t(run.max_ticks));
    };

    while (running() && pump_events(gamepad)) {
        const uint64_t now = SDL_GetTicksNS();
        const int due = clock.advance(now - last);
        last = now;
        for (int i = 0; i < due && running(); ++i) {
            if (run.input) {
                pad = run.input->next(pad);
            } else {
                RawInput raw = read_raw_input(SDL_GetKeyboardState(nullptr), gamepad);
                read_pointer(video, raw);
                pad = build_pad(raw, pad);
            }
            prog.update(pad, ticks);
            ++ticks;
            if (prog.status().outcome == Outcome::Error) break;
            if (run.observe) run.observe(ticks, pad, prog.state_hash());
            prog.take_audio(commands);
            audio.push(commands);
        }
        if (prog.status().outcome != Outcome::Running) break;
        prog.draw(fb);
        present(video, fb, argb);
    }
    if (gamepad) SDL_CloseGamepad(gamepad);
    return {result_name(prog.status().outcome), ticks, 0, prog.state_hash(), prog.budget_errors()};
}

}  // namespace

RunResult run_windowed(Program& prog, const WindowedRun& run) {
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "mother: SDL_Init failed: %s\n", SDL_GetError());
        return {"error", 0};
    }
    RunResult result{"error", 0};
    {
        Video video;
        if (open_video(video)) {
            result = run_loop(prog, run, video);
        } else {
            std::fprintf(stderr, "mother: could not open the window: %s\n", SDL_GetError());
        }
    }
    SDL_Quit();
    return result;
}

}  // namespace mother
