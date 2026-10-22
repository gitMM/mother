#include "input/pad.h"
#include "test.h"

using namespace mother;

TEST(pad_neutral_input_is_neutral) {
    const PadState p = build_pad(RawInput{}, PadState{});
    CHECK_EQ(p.stick_x, 0);
    CHECK_EQ(p.stick_y, 0);
    CHECK_EQ(p.held, 0);
    CHECK_EQ(p.pressed, 0);
}

TEST(pad_digital_direction_drives_stick) {
    RawInput raw;
    raw.left = true;
    raw.down = true;
    const PadState p = build_pad(raw, PadState{});
    CHECK_EQ(p.stick_x, -127);
    CHECK_EQ(p.stick_y, 127);
    CHECK(p.btn(Button::Left));
    CHECK(p.btn(Button::Down));
    CHECK(!p.btn(Button::Right));
    CHECK(!p.btn(Button::Up));
}

TEST(pad_digital_overrides_analog) {
    RawInput raw;
    raw.axis_x = 40;
    raw.right = true;
    CHECK_EQ(build_pad(raw, PadState{}).stick_x, 127);
}

TEST(pad_opposite_directions_fall_back_to_analog) {
    RawInput raw;
    raw.left = raw.right = true;
    raw.axis_x = -100;
    const PadState p = build_pad(raw, PadState{});
    CHECK_EQ(p.stick_x, -100);
    CHECK(p.btn(Button::Left));
}

TEST(pad_analog_sets_8way_from_half_deflection) {
    RawInput raw;
    raw.axis_x = 63;
    CHECK(!build_pad(raw, PadState{}).btn(Button::Right));
    raw.axis_x = 64;
    CHECK(build_pad(raw, PadState{}).btn(Button::Right));
    raw.axis_y = -64;
    CHECK(build_pad(raw, PadState{}).btn(Button::Up));
}

TEST(pad_clamps_axis_to_127) {
    RawInput raw;
    raw.axis_x = -128;
    CHECK_EQ(build_pad(raw, PadState{}).stick_x, -127);
}

TEST(pad_btnp_only_on_the_press_tick) {
    RawInput raw;
    raw.buttons[size_t(face_index(Button::A))] = true;
    const PadState p1 = build_pad(raw, PadState{});
    CHECK(p1.btn(Button::A));
    CHECK(p1.btnp(Button::A));
    const PadState p2 = build_pad(raw, p1);
    CHECK(p2.btn(Button::A));
    CHECK(!p2.btnp(Button::A));
    const PadState p3 = build_pad(RawInput{}, p2);
    CHECK(!p3.btn(Button::A));
    CHECK(build_pad(raw, p3).btnp(Button::A));
}

TEST(pad_directions_have_btnp) {
    RawInput raw;
    raw.up = true;
    const PadState p1 = build_pad(raw, PadState{});
    CHECK(p1.btnp(Button::Up));
    CHECK(!build_pad(raw, p1).btnp(Button::Up));
}

TEST(pad_every_face_button_maps_to_its_bit) {
    for (int i = 0; i < kFaceButtonCount; ++i) {
        RawInput raw;
        raw.buttons[size_t(i)] = true;
        const Button b = Button(int(Button::A) + i);
        CHECK_EQ(build_pad(raw, PadState{}).held, button_bit(b));
    }
}
