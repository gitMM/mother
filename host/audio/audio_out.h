#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include <SDL3/SDL_audio.h>

#include "audio/synth.h"

namespace mother {

// Plays audio through the default SDL device. The simulation thread pushes each tick's commands;
// SDL's audio thread renders them in order with its own Synth, 800 frames a tick, so the sound
// is the same sample stream a headless run renders. Without a device it stays silent.
class AudioOut {
public:
    explicit AudioOut(const AudioSetup& setup);
    ~AudioOut();
    AudioOut(const AudioOut&) = delete;
    AudioOut& operator=(const AudioOut&) = delete;

    void push(TickAudio& tick);  // takes the commands, leaving tick empty
    // A new synth with this setup for the ticks pushed from now on, in order with them.
    void restart(const AudioSetup& setup);

private:
    struct Queued {
        TickAudio tick;
        std::unique_ptr<AudioSetup> restart;  // applied before the tick, when set
    };

    static void SDLCALL feed(void* self, SDL_AudioStream* stream, int additional, int total);
    void fill(SDL_AudioStream* stream, int bytes);
    void render_next(std::vector<int16_t>& into, Queued& next);

    Synth synth_;                    // audio thread only
    std::vector<int16_t> pending_;   // audio thread only: rendered, not yet handed to SDL
    std::mutex mutex_;
    std::deque<Queued> queue_;       // guarded by mutex_
    std::unique_ptr<AudioSetup> restart_;  // simulation thread: for the next push
    SDL_AudioStream* stream_ = nullptr;
};

}  // namespace mother
