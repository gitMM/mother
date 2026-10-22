#include <array>
#include <initializer_list>

#include <SDL3/SDL_scancode.h>

#include "input/keymap.h"
#include "test.h"

using namespace mother;

namespace {

using Keys = std::array<bool, SDL_SCANCODE_COUNT>;

RawInput read(std::initializer_list<SDL_Scancode> down) {
    Keys keys{};
    for (SDL_Scancode k : down) keys[size_t(k)] = true;
    return read_raw_input(keys.data(), nullptr);
}

bool face(const RawInput& raw, Button b) {
    return raw.buttons[size_t(face_index(b))];
}

}  // namespace

TEST(keymap_arrows_and_wasd_drive_directions) {
    CHECK(read({SDL_SCANCODE_LEFT}).left);
    CHECK(read({SDL_SCANCODE_A}).left);
    CHECK(read({SDL_SCANCODE_RIGHT}).right);
    CHECK(read({SDL_SCANCODE_D}).right);
    CHECK(read({SDL_SCANCODE_UP}).up);
    CHECK(read({SDL_SCANCODE_W}).up);
    CHECK(read({SDL_SCANCODE_DOWN}).down);
    CHECK(read({SDL_SCANCODE_S}).down);
    const RawInput none = read({});
    CHECK(!none.left && !none.right && !none.up && !none.down);
}

TEST(keymap_face_buttons_follow_the_brief) {
    CHECK(face(read({SDL_SCANCODE_Z}), Button::A));
    CHECK(face(read({SDL_SCANCODE_X}), Button::B));
    CHECK(face(read({SDL_SCANCODE_C}), Button::X));
    CHECK(face(read({SDL_SCANCODE_V}), Button::Y));
    CHECK(face(read({SDL_SCANCODE_Q}), Button::L));
    CHECK(face(read({SDL_SCANCODE_E}), Button::R));
    CHECK(face(read({SDL_SCANCODE_RETURN}), Button::Start));
    // Z must press A and nothing else.
    const RawInput z = read({SDL_SCANCODE_Z});
    int pressed = 0;
    for (bool b : z.buttons) pressed += b ? 1 : 0;
    CHECK_EQ(pressed, 1);
}

TEST(keymap_axis_deadzone_and_range) {
    CHECK_EQ(quantise_axis(0), 0);
    CHECK_EQ(quantise_axis(8000), 0);
    CHECK_EQ(quantise_axis(-8000), 0);
    CHECK_EQ(quantise_axis(32767), 127);
    CHECK_EQ(quantise_axis(-32767), -127);
    CHECK_EQ(quantise_axis(-32768), -127);
    CHECK_EQ(quantise_axis(16000), -quantise_axis(-16000));
    CHECK(quantise_axis(16000) > 0 && quantise_axis(16000) < quantise_axis(24000));
}
