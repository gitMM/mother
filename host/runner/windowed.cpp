#include "runner/windowed.h"

#include <cstdio>
#include <vector>

#include <SDL3/SDL.h>
#define SDL_MAIN_HANDLED  // main() is ours; SDL_main.h only declares SDL_SetMainReady
#include <SDL3/SDL_main.h>

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
    const Viewport vp = fit_viewport(out_w, out_h);
    const SDL_FRect dst{float(vp.x), float(vp.y), float(vp.w), float(vp.h)};

    SDL_SetRenderDrawColor(v.renderer, 0, 0, 0, 255);
    SDL_RenderClear(v.renderer);
    SDL_RenderTexture(v.renderer, v.texture, nullptr, &dst);
    SDL_RenderPresent(v.renderer);
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

int run_loop(Program& prog, int64_t max_ticks, Video& video) {
    Framebuffer fb;
    std::vector<uint32_t> argb(size_t(kScreenW * kScreenH));
    SDL_Gamepad* gamepad = nullptr;
    TickClock clock;
    PadState pad;
    uint64_t tick = 0;
    uint64_t last = SDL_GetTicksNS();

    bool running = true;
    while (running) {
        running = pump_events(gamepad);
        const uint64_t now = SDL_GetTicksNS();
        const int due = clock.advance(now - last);
        last = now;
        for (int i = 0; i < due && running; ++i) {
            pad = build_pad(read_raw_input(SDL_GetKeyboardState(nullptr), gamepad), pad);
            prog.update(pad, tick);
            ++tick;
            if (max_ticks >= 0 && tick >= uint64_t(max_ticks)) running = false;
        }
        prog.draw(fb);
        present(video, fb, argb);
    }
    if (gamepad) SDL_CloseGamepad(gamepad);
    return 0;
}

}  // namespace

int run_windowed(Program& prog, int64_t max_ticks) {
    SDL_SetMainReady();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    int code = 1;
    {
        Video video;
        if (open_video(video)) {
            code = run_loop(prog, max_ticks, video);
        } else {
            std::fprintf(stderr, "could not open the window: %s\n", SDL_GetError());
        }
    }
    SDL_Quit();
    return code;
}

}  // namespace mother
