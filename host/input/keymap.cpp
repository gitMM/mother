#include "input/keymap.h"

#include <SDL3/SDL_scancode.h>

namespace mother {

namespace {

constexpr int kDeadzone = 8000;  // of 32767

struct FaceBind {
    Button button;
    SDL_Scancode key;
    SDL_GamepadButton pad;
};

constexpr FaceBind kFaceBinds[] = {
    {Button::A, SDL_SCANCODE_Z, SDL_GAMEPAD_BUTTON_SOUTH},
    {Button::B, SDL_SCANCODE_X, SDL_GAMEPAD_BUTTON_EAST},
    {Button::X, SDL_SCANCODE_C, SDL_GAMEPAD_BUTTON_WEST},
    {Button::Y, SDL_SCANCODE_V, SDL_GAMEPAD_BUTTON_NORTH},
    {Button::L, SDL_SCANCODE_Q, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER},
    {Button::R, SDL_SCANCODE_E, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER},
    {Button::Start, SDL_SCANCODE_RETURN, SDL_GAMEPAD_BUTTON_START},
};

}  // namespace

int8_t quantise_axis(int16_t value) {
    const int magnitude = value < 0 ? -int(value) : int(value);
    if (magnitude <= kDeadzone) return 0;
    int q = (magnitude - kDeadzone) * kAxisMax / (32767 - kDeadzone);
    if (q > kAxisMax) q = kAxisMax;
    return int8_t(value < 0 ? -q : q);
}

RawInput read_raw_input(const bool* keys, SDL_Gamepad* gamepad) {
    RawInput raw;
    raw.left = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
    raw.right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
    raw.up = keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W];
    raw.down = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S];
    for (const FaceBind& b : kFaceBinds) raw.buttons[size_t(face_index(b.button))] = keys[b.key];

    if (gamepad) {
        raw.axis_x = quantise_axis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX));
        raw.axis_y = quantise_axis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY));
        raw.left = raw.left || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
        raw.right = raw.right || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
        raw.up = raw.up || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP);
        raw.down = raw.down || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
        for (const FaceBind& b : kFaceBinds) {
            bool& slot = raw.buttons[size_t(face_index(b.button))];
            slot = slot || SDL_GetGamepadButton(gamepad, b.pad);
        }
    }
    return raw;
}

}  // namespace mother
