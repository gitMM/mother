#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

// A throwaway cart folder in the system temp directory, deleted when it goes out of scope.
class TempCart {
public:
    using Files = std::vector<std::pair<std::string, std::string>>;  // file name, contents

    TempCart(const std::string& name, const Files& files)
        : dir_(std::filesystem::temp_directory_path() / ("mother_test_" + name)) {
        std::error_code ec;
        std::filesystem::remove_all(dir_, ec);
        std::filesystem::create_directories(dir_);
        for (const auto& [file, text] : files) std::ofstream(dir_ / file, std::ios::binary) << text;
    }

    ~TempCart() {
        std::error_code ec;
        std::filesystem::remove_all(dir_, ec);
    }

    TempCart(const TempCart&) = delete;
    TempCart& operator=(const TempCart&) = delete;

    const std::filesystem::path& dir() const { return dir_; }

private:
    std::filesystem::path dir_;
};

inline const char* const kTestToml = "[cart]\nid = \"test\"\n";

// main.luau source with all four lifecycle functions; each body is Luau statements.
inline std::string lifecycle(const std::string& init, const std::string& update = "",
                             const std::string& draw = "") {
    return "function cart.init() " + init + " end\n" +
           "function cart.update() " + update + " end\n" +
           "function cart.draw() " + draw + " end\n" +
           "function cart.hash() return 0 end\n";
}
