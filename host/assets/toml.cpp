#include "assets/toml.h"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <set>
#include <utility>

#include "assets/text_lines.h"

namespace mother {

namespace {

constexpr int64_t kMaxExactInteger = int64_t(1) << 53;
constexpr int kMaxDepth = 16;  // nested arrays and inline tables

bool bare_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
}

TomlValue* find_in(TomlValue& table, std::string_view key) {
    for (size_t i = 0; i < table.keys.size(); ++i) {
        if (table.keys[i] == key) return &table.items[i];
    }
    return nullptr;
}

TomlValue* add(TomlValue& table, std::string key, TomlValue value) {
    table.keys.push_back(std::move(key));
    table.items.push_back(std::move(value));
    return &table.items.back();
}

// Parses the pieces of one line, left to right.
class LineParser {
public:
    LineParser(std::string_view text, int line) : s_(text), line_(line) {}

    // True at the end of the line or at a comment.
    bool at_end() {
        skip_space();
        return i_ >= s_.size() || s_[i_] == '#';
    }

    bool eat(char c) {
        skip_space();
        if (i_ < s_.size() && s_[i_] == c) {
            ++i_;
            return true;
        }
        return false;
    }

    // "[[" with nothing between the brackets.
    bool eat_double_bracket() {
        skip_space();
        if (s_.substr(i_, 2) != "[[") return false;
        i_ += 2;
        return true;
    }

    bool key(std::string& out) {
        skip_space();
        const size_t start = i_;
        while (i_ < s_.size() && bare_char(s_[i_])) ++i_;
        if (i_ == start) return fail("expected a name (letters, digits, _ and -)");
        out.assign(s_.substr(start, i_ - start));
        return true;
    }

    bool value(TomlValue& out, int depth = 0) {
        skip_space();
        out = TomlValue{};
        out.line = line_;
        if (i_ >= s_.size() || s_[i_] == '#') return fail("expected a value");
        if (depth > kMaxDepth) return fail("values nest too deeply");
        const char c = s_[i_];
        if (c == '"') return string_value(out);
        if (c == '[') return array_value(out, depth);
        if (c == '{') return table_value(out, depth);
        if (word("true") || word("false")) {
            out.type = TomlValue::Type::Bool;
            out.boolean = c == 't';
            return true;
        }
        return number_value(out);
    }

    const std::string& error() const { return error_; }

private:
    void skip_space() {
        while (i_ < s_.size() && (s_[i_] == ' ' || s_[i_] == '\t')) ++i_;
    }

    bool fail(std::string message) {
        error_ = std::move(message);
        return false;
    }

    // Consumes w when it is the next whole word.
    bool word(std::string_view w) {
        if (s_.substr(i_, w.size()) != w) return false;
        if (i_ + w.size() < s_.size() && bare_char(s_[i_ + w.size()])) return false;
        i_ += w.size();
        return true;
    }

    bool string_value(TomlValue& out) {
        out.type = TomlValue::Type::String;
        ++i_;  // opening quote
        while (i_ < s_.size()) {
            const char c = s_[i_++];
            if (c == '"') return true;
            if (static_cast<unsigned char>(c) < 0x20) return fail("control character in a string");
            if (c != '\\') {
                out.string += c;
                continue;
            }
            if (i_ >= s_.size()) break;
            switch (s_[i_++]) {
            case '"': out.string += '"'; break;
            case '\\': out.string += '\\'; break;
            case 'n': out.string += '\n'; break;
            case 't': out.string += '\t'; break;
            default: return fail("unknown escape in a string; use \\\" \\\\ \\n or \\t");
            }
        }
        return fail("unterminated string");
    }

    // [+-]digits[.digits][(e|E)[+-]digits]; an integer without the fraction and exponent.
    bool number_value(TomlValue& out) {
        const size_t start = i_;
        auto digits = [&] {
            const size_t from = i_;
            while (i_ < s_.size() && s_[i_] >= '0' && s_[i_] <= '9') ++i_;
            return i_ > from;
        };
        if (i_ < s_.size() && (s_[i_] == '+' || s_[i_] == '-')) ++i_;
        bool ok = digits();
        bool is_float = false;
        if (ok && i_ < s_.size() && s_[i_] == '.') {
            ++i_;
            ok = digits();
            is_float = true;
        }
        if (ok && i_ < s_.size() && (s_[i_] == 'e' || s_[i_] == 'E')) {
            ++i_;
            if (i_ < s_.size() && (s_[i_] == '+' || s_[i_] == '-')) ++i_;
            ok = digits();
            is_float = true;
        }
        if (!ok || (i_ < s_.size() && bare_char(s_[i_]))) {
            return fail("expected a value: a \"string\", number, true, false, [array] or { table }");
        }
        const std::string token(s_.substr(start, i_ - start));
        if (is_float) {
            out.type = TomlValue::Type::Float;
            out.number = std::strtod(token.c_str(), nullptr);
            if (!std::isfinite(out.number)) return fail("number out of range: " + token);
            return true;
        }
        out.type = TomlValue::Type::Integer;
        const char* first = token.c_str() + (token[0] == '+' ? 1 : 0);
        const std::errc ec = std::from_chars(first, token.c_str() + token.size(), out.integer).ec;
        if (ec != std::errc() || out.integer > kMaxExactInteger || out.integer < -kMaxExactInteger) {
            return fail("integer out of range (at most 2^53): " + token);
        }
        return true;
    }

    bool array_value(TomlValue& out, int depth) {
        out.type = TomlValue::Type::Array;
        ++i_;  // [
        while (!eat(']')) {
            TomlValue item;
            if (!value(item, depth + 1)) return false;
            out.items.push_back(std::move(item));
            if (eat(']')) return true;
            if (!eat(',')) return fail("expected ',' or ']' in an array");
        }
        return true;
    }

    bool table_value(TomlValue& out, int depth) {
        out.type = TomlValue::Type::Table;
        ++i_;  // {
        while (!eat('}')) {
            std::string k;
            if (!key(k)) return false;
            if (!eat('=')) return fail("expected '=' after " + k);
            TomlValue item;
            if (!value(item, depth + 1)) return false;
            if (out.find(k)) return fail("duplicate key " + k);
            add(out, std::move(k), std::move(item));
            if (eat('}')) return true;
            if (!eat(',')) return fail("expected ',' or '}' in an inline table");
        }
        return true;
    }

    std::string_view s_;
    size_t i_ = 0;
    int line_;
    std::string error_;
};

}  // namespace

const TomlValue* TomlValue::find(std::string_view key) const {
    return find_in(const_cast<TomlValue&>(*this), key);
}

const char* toml_type_name(TomlValue::Type type) {
    switch (type) {
    case TomlValue::Type::String: return "a string";
    case TomlValue::Type::Integer: return "an integer";
    case TomlValue::Type::Float: return "a float";
    case TomlValue::Type::Bool: return "a boolean";
    case TomlValue::Type::Array: return "an array";
    case TomlValue::Type::Table: return "a table";
    }
    return "a value";
}

bool parse_toml(std::string_view text, const std::string& filename, TomlValue& out, std::string& error) {
    TomlValue doc;
    doc.line = 1;
    TomlValue* current = &doc;
    std::set<std::string> array_tables;  // names made by [[name]]
    int n = 0;
    for (std::string_view raw : split_lines(text)) {
        ++n;
        LineParser p(raw, n);
        auto fail = [&](const std::string& message) {
            error = filename + ":" + std::to_string(n) + ": " + message;
            return false;
        };
        if (p.at_end()) continue;

        const bool array = p.eat_double_bracket();
        if (array || p.eat('[')) {
            std::string name;
            if (!p.key(name)) return fail(p.error());
            if (!p.eat(']') || (array && !p.eat(']'))) return fail(array ? "expected ']]'" : "expected ']'");
            if (!p.at_end()) return fail("unexpected text after [" + name + "]");
            TomlValue* list = find_in(doc, name);
            if (list && !(array && array_tables.count(name))) return fail(name + " is defined twice");
            TomlValue table;
            table.line = n;
            if (!array) {
                current = add(doc, name, std::move(table));
                continue;
            }
            if (!list) {
                TomlValue empty;
                empty.type = TomlValue::Type::Array;
                empty.line = n;
                list = add(doc, name, std::move(empty));
                array_tables.insert(name);
            }
            list->items.push_back(std::move(table));
            current = &list->items.back();
            continue;
        }

        std::string key;
        if (!p.key(key)) return fail(p.error());
        if (!p.eat('=')) return fail("expected '=' after " + key);
        TomlValue value;
        if (!p.value(value)) return fail(p.error());
        if (!p.at_end()) return fail("unexpected text after the value of " + key);
        if (current->find(key)) return fail("duplicate key " + key);
        add(*current, std::move(key), std::move(value));
    }
    out = std::move(doc);
    return true;
}

}  // namespace mother
