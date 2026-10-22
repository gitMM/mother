#include "audio/wav.h"

#include <array>

#include "audio/commands.h"

namespace mother {

namespace {

void put16(std::ofstream& out, uint32_t v) {
    const char b[2] = {char(v & 0xFF), char((v >> 8) & 0xFF)};
    out.write(b, 2);
}

void put32(std::ofstream& out, uint32_t v) {
    put16(out, v & 0xFFFF);
    put16(out, v >> 16);
}

void header(std::ofstream& out, uint32_t data_bytes) {
    out.write("RIFF", 4);
    put32(out, 36 + data_bytes);
    out.write("WAVEfmt ", 8);
    put32(out, 16);                    // fmt chunk size
    put16(out, 1);                     // PCM
    put16(out, 2);                     // channels
    put32(out, kSampleRate);
    put32(out, kSampleRate * 2 * 2);   // bytes per second
    put16(out, 4);                     // bytes per frame
    put16(out, 16);                    // bits per sample
    out.write("data", 4);
    put32(out, data_bytes);
}

}  // namespace

bool WavWriter::open(const std::filesystem::path& path) {
    out_.open(path, std::ios::binary | std::ios::trunc);
    data_bytes_ = 0;
    if (out_) header(out_, 0);
    return bool(out_);
}

void WavWriter::write(const int16_t* samples, size_t count) {
    for (size_t i = 0; i < count; ++i) put16(out_, uint16_t(samples[i]));
    data_bytes_ += uint32_t(count * 2);
}

bool WavWriter::close() {
    out_.seekp(0);
    header(out_, data_bytes_);
    out_.close();
    return !out_.fail();
}

}  // namespace mother
