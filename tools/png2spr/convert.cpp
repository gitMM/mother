#include "convert.h"

#include <fstream>
#include <iterator>
#include <utility>

#include "quantize.h"
#include "stb_image.h"

namespace mother {

bool load_png(const std::filesystem::path& path, RgbaImage& out, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "could not open " + path.string();
        return false;
    }
    const std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    int w = 0, h = 0, channels = 0;
    stbi_uc* px = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()), int(bytes.size()), &w,
                                        &h, &channels, 4);
    if (!px) {
        error = path.string() + ": not a readable PNG (" + stbi_failure_reason() + ")";
        return false;
    }
    out.w = w;
    out.h = h;
    out.px.assign(px, px + size_t(w) * size_t(h) * 4);
    stbi_image_free(px);
    return true;
}

bool image_to_spr(const RgbaImage& image, SprData& out, std::string& error) {
    if (image.w < 1 || image.h < 1 || image.w > kMaxPngW || image.h > kMaxPngH) {
        error = "the image is " + std::to_string(image.w) + "x" + std::to_string(image.h) +
                " px; it must be at most 256 wide and 768 tall (sheets 0-2)";
        return false;
    }
    SprData data;
    for (int y = 0; y < image.h; ++y) {
        for (int x = 0; x < image.w; ++x) {
            const uint8_t* p = &image.px[(size_t(y) * size_t(image.w) + size_t(x)) * 4];
            data.sheets.set(x, y, p[3] < 128 ? uint8_t(0) : nearest_index(p[0], p[1], p[2]));
        }
    }
    for (int n = 0; n * kSheetSize < image.h; ++n) data.defined[size_t(n)] = true;
    out = std::move(data);
    return true;
}

}  // namespace mother
