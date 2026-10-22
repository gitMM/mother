#include "vm/cart_source.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <system_error>

#include "assets/manifest.h"
#include "assets/map_format.h"
#include "assets/trk_format.h"

namespace mother {

namespace fs = std::filesystem;

namespace {

bool read_file(const fs::path& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return !in.bad();
}

}  // namespace

std::vector<fs::path> cart_folders(const fs::path& dir) {
    std::error_code ec;
    std::vector<fs::path> out;
    for (const fs::directory_entry& e : fs::directory_iterator(dir, ec)) {
        if (e.is_directory() && fs::is_regular_file(e.path() / "cart.toml")) out.push_back(e.path());
    }
    std::sort(out.begin(), out.end());
    return out;
}

bool valid_module_name(const std::string& name) {
    if (name.empty()) return false;
    for (char c : name) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) return false;
    }
    return true;
}

bool load_cart_source(const fs::path& dir, CartSource& out, std::string& error) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        error = "not a cart folder: " + dir.string();
        return false;
    }
    if (!fs::is_regular_file(dir / "cart.toml", ec)) {
        error = "missing cart.toml in " + dir.string();
        return false;
    }

    CartSource source;
    source.dir = dir;
    std::string toml;
    if (!read_file(dir / "cart.toml", toml)) {
        error = "could not read " + (dir / "cart.toml").string();
        return false;
    }
    if (!parse_manifest(toml, "cart.toml", source.manifest, error)) return false;

    size_t total = 0;
    for (const fs::directory_entry& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec) || entry.path().extension() != ".luau") continue;
        const std::string name = entry.path().stem().string();
        if (!valid_module_name(name)) {
            error = "bad module file name " + entry.path().filename().string() +
                    ": use letters, digits and _ only";
            return false;
        }
        std::string text;
        if (!read_file(entry.path(), text)) {
            error = "could not read " + entry.path().string();
            return false;
        }
        total += text.size();
        source.modules.emplace(name, std::move(text));
    }
    if (ec) {
        error = "could not list " + dir.string() + ": " + ec.message();
        return false;
    }
    if (source.modules.count("main") == 0) {
        error = "missing main.luau in " + dir.string();
        return false;
    }
    if (total > kMaxCartSource) {
        error = "cart source is " + std::to_string(total) + " bytes of Luau; the limit is " +
                std::to_string(kMaxCartSource);
        return false;
    }

    std::string text;
    if (fs::is_regular_file(dir / "sprites.spr", ec)) {
        if (!read_file(dir / "sprites.spr", text)) {
            error = "could not read " + (dir / "sprites.spr").string();
            return false;
        }
        if (!parse_spr(text, "sprites.spr", source.art, error)) return false;
    }
    if (fs::is_regular_file(dir / "map.map", ec)) {
        if (!read_file(dir / "map.map", text)) {
            error = "could not read " + (dir / "map.map").string();
            return false;
        }
        if (!parse_map(text, "map.map", source.map, error)) return false;
    }
    if (fs::is_regular_file(dir / "audio.trk", ec)) {
        if (!read_file(dir / "audio.trk", text)) {
            error = "could not read " + (dir / "audio.trk").string();
            return false;
        }
        if (!parse_trk(text, "audio.trk", source.tracker, error)) return false;
    }
    out = std::move(source);
    return true;
}

}  // namespace mother
