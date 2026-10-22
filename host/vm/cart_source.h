#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>

#include "assets/spr_format.h"
#include "assets/tilemap.h"
#include "assets/toml.h"
#include "assets/tracker.h"

namespace mother {

constexpr size_t kMaxCartSource = 64 * 1024;  // bytes of Luau per cart

// A cart folder read into memory. Nothing is read from disk afterwards.
struct CartSource {
    std::filesystem::path dir;
    TomlValue manifest;  // cart.toml, checked against the schema in assets/manifest.h
    // Every *.luau file in the folder itself (not subfolders), keyed by module name: the file
    // name without ".luau".
    std::map<std::string, std::string> modules;
    SprData art;          // sprites.spr; empty sheets and no ramps when the file is absent
    TileMap map;          // map.map; no layers and zero flags when the file is absent
    TrackerData tracker;  // audio.trk; no songs or sfx when the file is absent
};

// Reads a cart folder. It must hold a valid cart.toml and main.luau, every module name must be
// valid, the Luau source must total at most kMaxCartSource bytes, and sprites.spr, map.map and
// audio.trk, when present, must parse.
bool load_cart_source(const std::filesystem::path& dir, CartSource& out, std::string& error);

// Letters, digits and underscores only, so a module name can never leave the cart folder.
bool valid_module_name(const std::string& name);

}  // namespace mother
