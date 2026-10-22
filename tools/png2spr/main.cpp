#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "convert.h"

// png2spr <in.png> <out.spr>: quantises a PNG to the palette and writes a .spr file.

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr,
                     "usage: png2spr <in.png> <out.spr>\n"
                     "  in.png   at most 256 x 768 px; rows 0-255 become sheet 0, 256-511 sheet 1,\n"
                     "           512-767 sheet 2. Alpha below 128 is transparent (index 0).\n"
                     "  out.spr  overwritten\n");
        return 2;
    }
    std::string error;
    mother::RgbaImage image;
    mother::SprData data;
    if (!mother::load_png(argv[1], image, error) || !mother::image_to_spr(image, data, error)) {
        std::fprintf(stderr, "png2spr: %s\n", error.c_str());
        return 1;
    }
    const std::string text = mother::write_spr(data);
    std::ofstream out(std::filesystem::path(argv[2]), std::ios::binary | std::ios::trunc);
    if (!out.write(text.data(), std::streamsize(text.size()))) {
        std::fprintf(stderr, "png2spr: could not write %s\n", argv[2]);
        return 1;
    }
    return 0;
}
