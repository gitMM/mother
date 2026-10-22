#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <string>

namespace mother {

constexpr size_t kMaxCartSource = 64 * 1024;  // bytes of Luau per cart

// A cart folder read into memory: every *.luau file in the folder itself (not subfolders),
// keyed by module name, the file name without ".luau". Nothing is read from disk afterwards.
struct CartSource {
    std::filesystem::path dir;
    std::map<std::string, std::string> modules;
};

// Reads a cart folder. It must hold cart.toml and main.luau, every module name must be valid
// and the Luau source must total at most kMaxCartSource bytes.
bool load_cart_source(const std::filesystem::path& dir, CartSource& out, std::string& error);

// Letters, digits and underscores only, so a module name can never leave the cart folder.
bool valid_module_name(const std::string& name);

}  // namespace mother
