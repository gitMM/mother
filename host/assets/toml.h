#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mother {

// The TOML subset cart manifests use (grammar in DECISIONS.md):
//   # comment          (also after a value)
//   [name]             a table
//   [[name]]           one more table in an array of tables
//   key = value        value: "string" (escapes \" \\ \n \t), integer, float, true, false,
//                      [array, of, values] or { key = value, ... }, each on one line
// Names and keys are bare: letters, digits, '_' and '-'. Duplicate keys and tables are errors.
struct TomlValue {
    enum class Type : uint8_t { String, Integer, Float, Bool, Array, Table };

    Type type = Type::Table;
    int line = 0;  // where the value (or a table's header) is
    std::string string;
    int64_t integer = 0;  // |integer| <= 2^53, so it is exact as a Luau number
    double number = 0;    // finite
    bool boolean = false;
    std::vector<TomlValue> items;  // Array elements, or Table values (one per key)
    std::vector<std::string> keys; // Table keys, in file order

    // A table's value for key, or nullptr.
    const TomlValue* find(std::string_view key) const;
    bool is_number() const { return type == Type::Integer || type == Type::Float; }
    double as_number() const { return type == Type::Integer ? double(integer) : number; }
};

const char* toml_type_name(TomlValue::Type type);

// Parses a whole document into a Table. Errors are "filename:line: message".
bool parse_toml(std::string_view text, const std::string& filename, TomlValue& out, std::string& error);

}  // namespace mother
