#include "vm/cart_source.h"

#include <fstream>
#include <iterator>
#include <system_error>

namespace mother {

namespace fs = std::filesystem;

namespace {

bool read_file(const fs::path& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    out.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    return !in.bad();
}

}  // namespace

bool valid_module_name(const std::string& name) {
    if (name.empty()) return false;
    for (char c : name) {
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) return false;
    }
    return true;
}

bool load_cart_source(const fs::path& dir, CartSource& out, std::string& error) {
    std::error_code ec;
    if (!fs::is_directory(dir, ec)) {
        error = "not a cart folder: " + dir.string();
        return false;
    }
    if (!fs::is_regular_file(dir / "cart.toml", ec)) {
        error = "missing cart.toml in " + dir.string();
        return false;
    }

    CartSource source;
    source.dir = dir;
    size_t total = 0;
    for (const fs::directory_entry& entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec) || entry.path().extension() != ".luau") continue;
        const std::string name = entry.path().stem().string();
        if (!valid_module_name(name)) {
            error = "bad module file name " + entry.path().filename().string() +
                    ": use letters, digits and _ only";
            return false;
        }
        std::string text;
        if (!read_file(entry.path(), text)) {
            error = "could not read " + entry.path().string();
            return false;
        }
        total += text.size();
        source.modules.emplace(name, std::move(text));
    }
    if (ec) {
        error = "could not list " + dir.string() + ": " + ec.message();
        return false;
    }
    if (source.modules.count("main") == 0) {
        error = "missing main.luau in " + dir.string();
        return false;
    }
    if (total > kMaxCartSource) {
        error = "cart source is " + std::to_string(total) + " bytes of Luau; the limit is " +
                std::to_string(kMaxCartSource);
        return false;
    }
    out = std::move(source);
    return true;
}

}  // namespace mother
