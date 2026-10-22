#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <tuple>
#include <utility>

#include "assets/spr_format.h"
#include "assets/toml.h"
#include "assets/tracker.h"

namespace mother {

// Imported sfx and their instruments go to these slots, up to 63 (DECISIONS.md, M8).
constexpr int kImportSfx = 48;
constexpr int kImportInstruments = 48;

// One export of another cart, to run in this one (brief section 9, "Import harness").
struct ImportSource {
    std::string cart;        // the source cart's [cart] id
    std::string export_id;   // its [[exports]] id
    std::string archetype;
    TomlValue params;        // the export's params: a table, empty when it gives none
    SprData art;             // the source cart's sprites, to copy the export's tiles from
    TrackerData tracker;     // the source cart's audio, to copy the export's sfx from

    std::string label() const { return cart + ":" + export_id; }
};

// Reads `SOURCE:EXPORT` for a cart in host_dir: SOURCE is a cart folder next to the host cart, or
// a path to one. The export must be in its manifest's [[exports]].
bool load_import(const std::filesystem::path& host_dir, const std::string& spec, ImportSource& out,
                 std::string& error);

// Copies imports' art and sounds into a cart, each at most once: tile blocks into sheet 3, packed
// in rows of 32 tiles from its top-left; sfx into slots 48-63, with the instruments they use in
// instruments 48-63. An sfx note that relied on the default instrument 0 is given the copy of
// the source's instrument 0. Wavetables aren't copied.
class ImportAssets {
public:
    // The first tile of the copy of the w x h tile block at `tile` (0-3071) in src, or -1 when
    // the block doesn't fit or sheet 3 is full.
    int copy_tiles(const ImportSource& src, int tile, int w, int h, SpriteSheets& dst);
    // The slot of the copy of src's sfx n, or -1 when src has no sfx n or the slots are full.
    int copy_sfx(const ImportSource& src, int n, TrackerData& dst);

private:
    int copy_instrument(const ImportSource& src, int inst, TrackerData& dst);

    std::map<std::tuple<std::string, int, int, int>, int> tiles_;  // (cart, tile, w, h) -> tile
    std::map<std::pair<std::string, int>, int> sfx_;               // (cart, sfx) -> slot
    std::map<std::pair<std::string, int>, int> instruments_;       // (cart, inst) -> slot
    int col_ = 0, row_ = 0, shelf_ = 0;                            // the packer, in tiles
    int next_sfx_ = kImportSfx, next_instrument_ = kImportInstruments;
};

}  // namespace mother
