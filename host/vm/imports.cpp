#include "vm/imports.h"

#include "vm/cart_source.h"

namespace mother {

namespace fs = std::filesystem;

bool load_import(const fs::path& host_dir, const std::string& spec, ImportSource& out, std::string& error) {
    const size_t colon = spec.rfind(':');
    if (colon == std::string::npos || colon == 0 || colon + 1 == spec.size()) {
        error = "an import is SOURCE:EXPORT, e.g. breakout:ball; got " + spec;
        return false;
    }
    const std::string source = spec.substr(0, colon);
    const std::string export_id = spec.substr(colon + 1);
    const bool is_path = source.find('/') != std::string::npos || source.find('\\') != std::string::npos;
    const fs::path dir = is_path ? fs::path(source) : host_dir.lexically_normal().parent_path() / source;

    CartSource cart;
    if (!load_cart_source(dir, cart, error)) {
        error = "import " + spec + ": " + error;
        return false;
    }
    const TomlValue* exports = cart.manifest.find("exports");
    const TomlValue* found = nullptr;
    for (size_t i = 0; exports && i < exports->items.size(); ++i) {
        if (exports->items[i].find("id")->string == export_id) found = &exports->items[i];
    }
    if (!found) {
        error = "import " + spec + ": " + source + "'s cart.toml has no [[exports]] with id " + export_id;
        return false;
    }
    out.cart = cart.manifest.find("cart")->find("id")->string;
    out.export_id = export_id;
    out.archetype = found->find("archetype")->string;
    const TomlValue* params = found->find("params");
    out.params = params ? *params : TomlValue{};
    out.art = std::move(cart.art);
    out.tracker = std::move(cart.tracker);
    return true;
}

int ImportAssets::copy_tiles(const ImportSource& src, int tile, int w, int h, SpriteSheets& dst) {
    if (tile < 0 || tile >= kCartTiles || w < 1 || h < 1 || w > kTilesPerRow) return -1;
    const auto key = std::make_tuple(src.cart, tile, w, h);
    if (auto it = tiles_.find(key); it != tiles_.end()) return it->second;
    if (col_ + w > kTilesPerRow) col_ = 0, row_ += shelf_, shelf_ = 0;
    if (row_ + h > kTilesPerRow) return -1;
    const int first = kCartTiles + row_ * kTilesPerRow + col_;
    const int sx = (tile % kTilesPerRow) * kTileSize, sy = (tile / kTilesPerRow) * kTileSize;
    const int dx = (first % kTilesPerRow) * kTileSize, dy = (first / kTilesPerRow) * kTileSize;
    for (int y = 0; y < h * kTileSize; ++y) {
        for (int x = 0; x < w * kTileSize; ++x) dst.set(dx + x, dy + y, src.art.sheets.get(sx + x, sy + y));
    }
    col_ += w;
    if (h > shelf_) shelf_ = h;
    tiles_[key] = first;
    return first;
}

int ImportAssets::copy_instrument(const ImportSource& src, int inst, TrackerData& dst) {
    const auto key = std::make_pair(src.cart, inst);
    if (auto it = instruments_.find(key); it != instruments_.end()) return it->second;
    if (next_instrument_ >= kInstruments) return -1;
    const int slot = next_instrument_++;
    dst.instruments[size_t(slot)] = src.tracker.instruments[size_t(inst)];
    instruments_[key] = slot;
    return slot;
}

int ImportAssets::copy_sfx(const ImportSource& src, int n, TrackerData& dst) {
    if (n < 0 || n >= kSfxCount || src.tracker.sfx[size_t(n)].speed == 0) return -1;
    const auto key = std::make_pair(src.cart, n);
    if (auto it = sfx_.find(key); it != sfx_.end()) return it->second;
    if (next_sfx_ >= kSfxCount) return -1;
    Sfx copy = src.tracker.sfx[size_t(n)];
    bool chosen = false;  // has a row chosen an instrument yet?
    for (Cell& cell : copy.rows) {
        if (cell.inst < 0 && cell.note > 0 && !chosen) cell.inst = 0;  // the default it relied on
        if (cell.inst >= 0) {
            const int slot = copy_instrument(src, cell.inst, dst);
            if (slot < 0) return -1;
            cell.inst = int8_t(slot);
            chosen = true;
        }
    }
    const int slot = next_sfx_++;
    dst.sfx[size_t(slot)] = std::move(copy);
    sfx_[key] = slot;
    return slot;
}

}  // namespace mother
