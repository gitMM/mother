#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "assets/spr_format.h"

namespace mother {

constexpr int kMaxPngW = kSheetSize;                // 256
constexpr int kMaxPngH = kSheetSize * kCartSheets;  // 768: sheets 0-2 stacked

struct RgbaImage {
    int w = 0, h = 0;
    std::vector<uint8_t> px;  // row-major, 4 bytes per pixel
};

// Reads and decodes a PNG file to 8-bit RGBA.
bool load_png(const std::filesystem::path& path, RgbaImage& out, std::string& error);

// Quantises an image into sprite sheets. Pixel (x, y) lands at atlas (x, y), so an image taller
// than 256 px fills sheets 1 and 2; every sheet the image touches is marked defined. Alpha below
// 128 becomes index 0 (transparent); every other pixel becomes the nearest palette colour in
// OKLab from 1-63. At most 256 x 768 px.
bool image_to_spr(const RgbaImage& image, SprData& out, std::string& error);

}  // namespace mother
