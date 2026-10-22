#include <string>

#include "assets/toml.h"
#include "test.h"

using namespace mother;

namespace {

using Type = TomlValue::Type;

TomlValue parse(const std::string& text) {
    TomlValue doc;
    std::string error;
    if (!parse_toml(text, "t.toml", doc, error)) std::printf("  unexpected error: %s\n", error.c_str());
    return doc;
}

std::string error_of(const std::string& text) {
    TomlValue doc;
    std::string error;
    if (parse_toml(text, "t.toml", doc, error)) return "(parsed)";
    return error;
}

}  // namespace

TEST(toml_reads_scalars) {
    const TomlValue doc = parse("a = \"hi \\\"you\\\"\\n\"\nb = -42\nc = 0.25\nd = 1e3\ne = true\nf = false # note\n");
    CHECK(doc.type == Type::Table);
    CHECK_EQ(doc.find("a")->string, std::string("hi \"you\"\n"));
    CHECK(doc.find("b")->type == Type::Integer && doc.find("b")->integer == -42);
    CHECK(doc.find("c")->type == Type::Float && doc.find("c")->number == 0.25);
    CHECK(doc.find("d")->type == Type::Float && doc.find("d")->number == 1000.0);
    CHECK(doc.find("e")->boolean && !doc.find("f")->boolean);
    CHECK_EQ(doc.find("f")->line, 6);
    CHECK(doc.find("g") == nullptr);
}

TEST(toml_reads_tables_and_arrays_of_tables) {
    const TomlValue doc = parse(
        "# header comment\n"
        "[cart]\nid = \"x\"\n\n"
        "[[points]]\nx = 1\n[[points]]\nx = 2\n"
        "[env]\nbounds = { left = \"solid\", top = \"kill\" }\nlist = [1, 2.5, \"three\", [4],]\n");
    CHECK_EQ(doc.keys.size(), size_t(3));
    CHECK_EQ(doc.find("cart")->find("id")->string, std::string("x"));
    const TomlValue* points = doc.find("points");
    CHECK(points->type == Type::Array && points->items.size() == 2);
    CHECK_EQ(points->items[1].find("x")->integer, int64_t(2));
    CHECK_EQ(points->items[1].line, 7);
    const TomlValue* bounds = doc.find("env")->find("bounds");
    CHECK(bounds->type == Type::Table && bounds->keys[1] == "top" && bounds->items[1].string == "kill");
    const TomlValue* list = doc.find("env")->find("list");
    CHECK_EQ(list->items.size(), size_t(4));
    CHECK(list->items[1].as_number() == 2.5 && list->items[3].items[0].integer == 4);
    CHECK(parse("e = {}\nf = []\n").find("e")->keys.empty());
}

TEST(toml_reads_crlf_and_blank_lines) {
    const TomlValue doc = parse("[a]\r\n\r\nx = 1\r\n");
    CHECK_EQ(doc.find("a")->find("x")->integer, int64_t(1));
}

TEST(toml_errors_name_the_line) {
    CHECK_EQ(error_of("a = 1\na = 2\n"), std::string("t.toml:2: duplicate key a"));
    CHECK_EQ(error_of("[a]\n[a]\n"), std::string("t.toml:2: a is defined twice"));
    CHECK_EQ(error_of("a = 1\n[[a]]\n"), std::string("t.toml:2: a is defined twice"));
    CHECK_EQ(error_of("[[a]]\n[a]\n"), std::string("t.toml:2: a is defined twice"));
    CHECK_EQ(error_of("x\n"), std::string("t.toml:1: expected '=' after x"));
    CHECK_EQ(error_of("x = \n"), std::string("t.toml:1: expected a value"));
    CHECK_EQ(error_of("x = \"abc\n"), std::string("t.toml:1: unterminated string"));
    CHECK(error_of("x = \"a\\q\"\n").find("unknown escape") != std::string::npos);
    CHECK(error_of("x = 1 2\n").find("unexpected text after the value of x") != std::string::npos);
    CHECK(error_of("x = 9007199254740993\n").find("integer out of range") != std::string::npos);
    CHECK(error_of("x = 1e999\n").find("number out of range") != std::string::npos);
    CHECK(error_of("x = yes\n").find("expected a value") != std::string::npos);
    CHECK(error_of("x = 1.\n").find("expected a value") != std::string::npos);
    CHECK(error_of("x = { a = 1 b = 2 }\n").find("expected ',' or '}'") != std::string::npos);
    CHECK(error_of("x = { a = 1, a = 2 }\n").find("duplicate key a") != std::string::npos);
    CHECK(error_of("x = [1 2]\n").find("expected ',' or ']'") != std::string::npos);
    CHECK(error_of("[a b]\n").find("expected ']'") != std::string::npos);
    CHECK(error_of("[a] x = 1\n").find("unexpected text after [a]") != std::string::npos);
    CHECK(error_of("a.b = 1\n").find("expected '=' after a") != std::string::npos);
    CHECK(error_of(std::string(40, '[') + "\n").find("expected a name") != std::string::npos);
    CHECK(error_of("x = " + std::string(40, '[') + "\n").find("nest too deeply") != std::string::npos);
}
