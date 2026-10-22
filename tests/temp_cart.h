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

// A cart.toml for test carts: Toml{}.text() is the plainest valid manifest; set fields to change
// [env] values, the [objective] body, or add sections after it.
struct Toml {
    std::string gravity = "none", surface = "none", scroll = "none", avatar = "none", spawn = "none";
    std::string deploy = "false";
    std::string bounds = "solid";  // all four edges, or a whole inline table starting with '{'
    std::string objective = "type = \"survive\"\nticks = 36000";
    std::string extra;  // more sections: [avatar], [[spawn_points]], [[exports]]

    std::string text() const {
        const std::string b = bounds.rfind('{', 0) == 0
                                  ? bounds
                                  : "{ left = \"" + bounds + "\", right = \"" + bounds + "\", top = \"" + bounds +
                                        "\", bottom = \"" + bounds + "\" }";
        return "[cart]\nid = \"test\"\ntitle = \"TEST\"\nfamily = \"action\"\n\n[env]\ngravity = \"" + gravity +
               "\"\nsurface = \"" + surface + "\"\nscroll = \"" + scroll + "\"\navatar = \"" + avatar +
               "\"\nspawn = \"" + spawn + "\"\ndeploy = " + deploy + "\nbounds = " + b + "\n\n[objective]\n" +
               objective + "\n" + extra;
    }
};

inline const std::string kTestToml = Toml{}.text();

// A .map flags section: 3072 lines of "00", except tile `tile`, which gets `hex`.
inline std::string flags_block(int tile = -1, const char* hex = "00") {
    std::string text = "flags\n";
    for (int t = 0; t < 3072; ++t) text += std::string(t == tile ? hex : "00") + "\n";
    return text;
}

// main.luau source with all four lifecycle functions; each body is Luau statements.
inline std::string lifecycle(const std::string& init, const std::string& update = "",
                             const std::string& draw = "") {
    return "function cart.init() " + init + " end\n" +
           "function cart.update() " + update + " end\n" +
           "function cart.draw() " + draw + " end\n" +
           "function cart.hash() return 0 end\n";
}
