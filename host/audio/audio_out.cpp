#include "audio/audio_out.h"

#include <array>
#include <cstdio>

#include <SDL3/SDL.h>

namespace mother {

namespace {

// More queued ticks than this means the device fell behind: the extra ticks are rendered (so
// no command is lost) but their samples are dropped, keeping latency near 100 ms.
constexpr size_t kMaxQueuedTicks = 6;

}  // namespace

AudioOut::AudioOut(const AudioSetup& setup) : synth_(setup) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::fprintf(stderr, "mother: no audio: %s\n", SDL_GetError());
        return;
    }
    const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, kSampleRate};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, feed, this);
    if (!stream_) {
        std::fprintf(stderr, "mother: no audio device: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return;
    }
    SDL_ResumeAudioStreamDevice(stream_);
}

AudioOut::~AudioOut() {
    if (!stream_) return;
    SDL_DestroyAudioStream(stream_);  // stops the callback before the members go
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void AudioOut::push(TickAudio& tick) {
    Queued next;
    next.tick.swap(tick);
    next.restart = std::move(restart_);
    if (!stream_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(next));
}

void AudioOut::restart(const AudioSetup& setup) {
    restart_ = std::make_unique<AudioSetup>(setup);
}

void SDLCALL AudioOut::feed(void* self, SDL_AudioStream* stream, int additional, int) {
    static_cast<AudioOut*>(self)->fill(stream, additional);
}

void AudioOut::fill(SDL_AudioStream* stream, int bytes) {
    const size_t wanted = size_t(bytes) / sizeof(int16_t);
    Queued next;
    std::vector<int16_t> dropped;
    for (;;) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.size() <= kMaxQueuedTicks) break;
            next = std::move(queue_.front());
            queue_.pop_front();
        }
        dropped.clear();
        render_next(dropped, next);
    }
    while (pending_.size() < wanted) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty()) break;
            next = std::move(queue_.front());
            queue_.pop_front();
        }
        render_next(pending_, next);
    }
    if (pending_.size() < wanted) pending_.resize(wanted, 0);  // the simulation is behind: silence
    SDL_PutAudioStreamData(stream, pending_.data(), int(wanted * sizeof(int16_t)));
    pending_.erase(pending_.begin(), pending_.begin() + std::ptrdiff_t(wanted));
}

void AudioOut::render_next(std::vector<int16_t>& into, Queued& next) {
    if (next.restart) synth_ = Synth(*next.restart);
    std::array<int16_t, kTickFrames * 2> samples{};
    synth_.render_tick(next.tick, samples.data());
    into.insert(into.end(), samples.begin(), samples.end());
}

}  // namespace mother
