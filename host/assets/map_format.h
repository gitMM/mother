#pragma once

#include <string>
#include <string_view>

#include "assets/tilemap.h"

namespace mother {

// The contents of a .map file (brief section 4). Grammar, blank lines allowed between blocks:
//   layer N W H   N = 0 or 1, W and H = 1..256, followed by H lines of W tile indices, each
//                 exactly 4 decimal digits (0000-3071), separated by spaces; each layer at most once
//   flags         followed by exactly 3072 lines of two hex digits, the flags of tiles 0-3071;
//                 required, exactly once
bool parse_map(std::string_view text, const std::string& filename, TileMap& out, std::string& error);

}  // namespace mother
