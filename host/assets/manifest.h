#pragma once

#include <string>
#include <string_view>

#include "assets/toml.h"

namespace mother {

// Parses cart.toml (brief section 6) and checks it against the manifest schema:
//   [cart]          id (letters, digits, _), title, family = action|platform|driving|shooter|puzzle
//   [env]           gravity = none|down, surface = none|ground, scroll = none|follow|auto,
//                   avatar = none|up|lr|8dir|heading, spawn = none|world|camera, deploy = bool,
//                   bounds = { left, right, top, bottom } each solid|kill|despawn
//   [objective]     type = kill_all | survive (ticks) | race (laps) | escape (exit = { x, y, w, h })
//                   | collect (archetype)
//   [avatar]        archetype, damage = hp|stun, hp, x, y, optional params = { ... };
//                   required when env.avatar isn't none, forbidden when it is
//   [[spawn_points]] x, y; at least one when env.spawn isn't none, none when it is
//   [[exports]]     id (unique), archetype, optional params = { ... }
// Every key above is required unless marked optional, and unknown keys are errors. Archetype
// names are only checked for their spelling here; MOTHERLIB checks that they exist.
bool parse_manifest(std::string_view text, const std::string& filename, TomlValue& out, std::string& error);

}  // namespace mother
