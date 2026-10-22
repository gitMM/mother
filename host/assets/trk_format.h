#pragma once

#include <string>
#include <string_view>

#include "assets/tracker.h"

namespace mother {

// The contents of a .trk file (brief section 4). Lines starting with # are comments and are
// ignored everywhere. Blocks start with a header line; instrument numbers are two hex digits
// (as in cells), every other number is decimal.
//
//   wavetable N          N 0-31; the next line holds 32 samples, -128..127
//   instrument XX        XX 00-3F; key lines follow until a blank line or the next header:
//     wave W             square12 | square25 | square50 | saw | triangle | noise | table N
//     adsr A D S R       attack, decay, release in ticks 0-255; sustain level 0-64
//     volume V           0-64
//     pan P              0-255, 128 = centre
//     slide S            -128..127 sixteenths of a semitone per tick
//     vibrato SPEED DEPTH  0-255 LFO steps (of 256) per tick; 0-255 sixteenths of a semitone
//   sfx N SPEED          N 0-63, SPEED ticks per row 1-255; 1-64 cell lines follow (see
//                        trk_cell.h) until a blank line or the next header
//   pattern N            N 0-63; exactly 64 lines follow, each six cells separated by |
//   song N TEMPO MODE    N 0-7, TEMPO ticks per row 1-255, MODE loop | once; the next line holds
//                        1-64 pattern numbers 0-63
//   fx                   global effects; key lines follow until a blank line or the next header:
//     delay MS FEEDBACK MIX   MS 0-1000, FEEDBACK and MIX 0-64 (MIX 0 = off)
//     lowpass CUTOFF          0-255 (255 = open)
//     bitcrush BITS RATE      BITS 1-16 (16 = off), RATE 1-16 sample-and-hold (1 = off)
//
// Every block and key may appear once. Instruments that are never defined play with the
// VoiceParams defaults (square50, no envelope, full volume, centre).
bool parse_trk(std::string_view text, const std::string& filename, TrackerData& out, std::string& error);

}  // namespace mother
