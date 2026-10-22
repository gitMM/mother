#pragma once

#include <string>
#include <string_view>

#include "assets/tracker.h"

namespace mother {

// One tracker cell as text: NOTE INST VOL FX, separated by spaces, e.g. "C-4 01 40 V42".
//   NOTE  C-4, C#4 ... B-7 (sharps only, octaves 0-7), --- empty, === note off
//   INST  two hex digits 00-3F, or .. for none (the channel keeps its last instrument)
//   VOL   two hex digits 00-40, or .. for none (the instrument's volume on a new note)
//   FX    ... for none, or a letter and two hex digits:
//           Pxx pan (00 left, 80 centre, FF right)
//           Uxx slide up, Dxx slide down: xx sixteenths of a semitone per tick
//           Vxy vibrato: speed x*8 LFO steps per tick, depth y*4 sixteenths of a semitone
//           Txx set the music tempo to xx ticks per row (music only)
bool parse_cell(std::string_view text, Cell& out, std::string& error);
std::string format_cell(const Cell& cell);

// The pieces on their own, as song_get/song_set and instrument headers use them.
bool parse_note(std::string_view text, int& note);  // -> 0, kNoteOff or 1-96
bool parse_instrument_number(std::string_view text, int& n);  // two hex digits 00-3F
bool parse_fx(std::string_view text, char& fx, uint8_t& param);
std::string format_note(int note);
std::string format_fx(char fx, uint8_t param);

}  // namespace mother
