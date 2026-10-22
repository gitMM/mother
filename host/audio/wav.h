#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace mother {

// Streams 48 kHz 16-bit stereo samples to a WAV file; close() fills in the header sizes.
class WavWriter {
public:
    bool open(const std::filesystem::path& path);
    void write(const int16_t* samples, size_t count);  // interleaved left, right
    bool close();

private:
    std::ofstream out_;
    uint32_t data_bytes_ = 0;
};

}  // namespace mother
