#include "audio/sequencer.h"

#include <algorithm>

namespace mother {

namespace {

AudioCommand command(AudioCommand::Kind kind, int voice, int32_t value = 0) {
    AudioCommand c;
    c.kind = kind;
    c.voice = uint8_t(voice);
    c.value = value;
    return c;
}

// Row effects that change a voice parameter: P pan, U/D slide, V vibrato.
void apply_fx(const Cell& cell, VoiceParams& p) {
    switch (cell.fx) {
    case 'P': p.pan = cell.param; break;
    case 'U': p.slide = int8_t(std::min<int>(cell.param, 127)); break;
    case 'D': p.slide = int8_t(-std::min<int>(cell.param, 128)); break;
    case 'V':
        p.vibrato_speed = uint8_t((cell.param >> 4) * 8);
        p.vibrato_depth = uint8_t((cell.param & 15) * 4);
        break;
    default: break;
    }
}

}  // namespace

void Sequencer::sfx(int n, int voice) {
    if (voice < 0) {
        // The lowest voice without an sfx; when all six are busy, the one whose sfx started first.
        voice = 0;
        for (int v = 1; v < kCartVoices; ++v) {
            if (sfx_[size_t(v)].order < sfx_[size_t(voice)].order) voice = v;
        }
        for (int v = kCartVoices - 1; v >= 0; --v) {
            if (sfx_[size_t(v)].n < 0) voice = v;
        }
    }
    sfx_[size_t(voice)] = {n, 0, 0, 0, ++sfx_started_};
}

void Sequencer::music(int song, int fade_ticks) {
    if (song < 0) {
        if (song_ < 0) return;
        if (fade_ticks <= 0) {
            end_music(pending_);
        } else {
            fade_from_ = gain_, fade_to_ = 0, fade_len_ = fade_ticks, fade_t_ = 0;
            stop_after_fade_ = true;
        }
        return;
    }
    if (song_ >= 0) end_music(pending_);
    song_ = song;
    position_ = 0, next_row_ = 0, wait_ = 0, rows_played_ = 0, row_ = 0;
    tempo_ = data_->songs[size_t(song)].tempo;
    last_row_ = false;
    music_inst_.fill(0);
    stop_after_fade_ = false;
    if (fade_ticks > 0) {
        gain_ = 0;
        fade_from_ = 0, fade_to_ = kMusicGainMax, fade_len_ = fade_ticks, fade_t_ = 0;
    } else {
        gain_ = kMusicGainMax;
        fade_len_ = 0;
    }
    pending_.push_back(command(AudioCommand::Kind::MusicGain, 0, gain_));
}

void Sequencer::stop(int voice) {
    if (voice < 0) {
        if (song_ >= 0) song_ = -1, fade_len_ = 0;
        for (int v = 0; v < kCartVoices; ++v) {
            sfx_[size_t(v)].n = -1;
            pending_.push_back(command(AudioCommand::Kind::Stop, v));
        }
        return;
    }
    sfx_[size_t(voice)].n = -1;
    pending_.push_back(command(AudioCommand::Kind::Stop, voice));
}

bool Sequencer::beat(int& bar, int& beat, int& row) const {
    if (song_ < 0) return false;
    const int total = std::max(rows_played_ - 1, 0);
    bar = total / (kRowsPerBeat * kBeatsPerBar);
    beat = (total / kRowsPerBeat) % kBeatsPerBar;
    row = row_;
    return true;
}

void Sequencer::tick(TickAudio& out) {
    out.insert(out.end(), pending_.begin(), pending_.end());
    pending_.clear();
    if (fade_len_ > 0) {
        ++fade_t_;
        gain_ = fade_from_ + int(int64_t(fade_to_ - fade_from_) * fade_t_ / fade_len_);
        out.push_back(command(AudioCommand::Kind::MusicGain, 0, gain_));
        if (fade_t_ == fade_len_) {
            fade_len_ = 0;
            if (stop_after_fade_) end_music(out);
        }
    }
    music_tick(out);
    sfx_tick(out);
}

void Sequencer::music_tick(TickAudio& out) {
    if (song_ < 0) return;
    const Song& song = data_->songs[size_t(song_)];
    if (wait_ == 0) {
        const PatternRow& cells = data_->patterns[song.order[size_t(position_)]][size_t(next_row_)];
        for (const Cell& cell : cells) {
            if (cell.fx == 'T' && cell.param > 0) tempo_ = cell.param;
        }
        for (int c = 0; c < kChannels; ++c) {
            int& inst = music_inst_[size_t(c)];
            if (sfx_[size_t(c)].n < 0) {
                trigger(cells[size_t(c)], c, true, inst, out);
            } else if (cells[size_t(c)].inst >= 0) {
                inst = cells[size_t(c)].inst;  // keep the channel's instrument current under an sfx
            }
        }
        row_ = next_row_;
        ++rows_played_;
        wait_ = tempo_;
        if (++next_row_ == kPatternRows) {
            next_row_ = 0;
            if (++position_ == int(song.order.size())) {
                position_ = 0;
                last_row_ = !song.loop;
            }
        }
    }
    if (--wait_ == 0 && last_row_) end_music(out);
}

void Sequencer::sfx_tick(TickAudio& out) {
    for (int v = 0; v < kVoices; ++v) {
        SfxPlay& play = sfx_[size_t(v)];
        if (play.n < 0) continue;
        const Sfx& s = data_->sfx[size_t(play.n)];
        if (play.wait == 0) {
            if (play.row == int(s.rows.size())) {
                play.n = -1;
                out.push_back(command(AudioCommand::Kind::Release, v));
                continue;
            }
            trigger(s.rows[size_t(play.row)], v, false, play.inst, out);
            ++play.row;
            play.wait = s.speed;
        }
        --play.wait;
    }
}

void Sequencer::trigger(const Cell& cell, int voice, bool music, int& inst, TickAudio& out) const {
    if (cell.inst >= 0) inst = cell.inst;
    if (cell.note > 0) {
        AudioCommand c = command(AudioCommand::Kind::NoteOn, voice);
        c.music = music;
        c.pitch = int16_t((cell.note - 1) * 16);
        c.params = data_->instruments[size_t(inst)];
        if (cell.vol >= 0) c.params.volume = uint8_t(cell.vol);
        apply_fx(cell, c.params);
        out.push_back(c);
        return;
    }
    if (cell.note == kNoteOff) {
        out.push_back(command(AudioCommand::Kind::Release, voice));
        return;
    }
    if (cell.vol >= 0) out.push_back(command(AudioCommand::Kind::Volume, voice, cell.vol));
    VoiceParams p;
    apply_fx(cell, p);
    switch (cell.fx) {
    case 'P': out.push_back(command(AudioCommand::Kind::Pan, voice, p.pan)); break;
    case 'U':
    case 'D': out.push_back(command(AudioCommand::Kind::Slide, voice, p.slide)); break;
    case 'V': out.push_back(command(AudioCommand::Kind::Vibrato, voice, p.vibrato_speed << 8 | p.vibrato_depth)); break;
    default: break;
    }
}

void Sequencer::end_music(TickAudio& out) {
    song_ = -1;
    fade_len_ = 0;
    for (int c = 0; c < kChannels; ++c) {
        if (sfx_[size_t(c)].n < 0) out.push_back(command(AudioCommand::Kind::Release, c));
    }
}

}  // namespace mother
