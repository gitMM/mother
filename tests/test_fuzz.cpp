#include "input/fuzz.h"
#include "test.h"

using namespace mother;

namespace {

bool same(const RawInput& a, const RawInput& b) {
    return a.left == b.left && a.right == b.right && a.up == b.up && a.down == b.down && a.buttons == b.buttons;
}

}  // namespace

TEST(fuzz_same_seed_same_input) {
    FuzzInput a(9), b(9), c(10);
    bool differs = false;
    for (int t = 0; t < 3600; ++t) {
        const RawInput x = a.next();
        CHECK(same(x, b.next()));
        differs = differs || !same(x, c.next());
    }
    CHECK(differs);
}

TEST(fuzz_input_is_plausible) {
    FuzzInput fuzz(3);
    int a_presses = 0, start_presses = 0, dir_changes = 0, opposite = 0;
    RawInput prev;
    for (int t = 0; t < 36000; ++t) {
        const RawInput raw = fuzz.next();
        a_presses += raw.buttons[size_t(face_index(Button::A))] && !prev.buttons[size_t(face_index(Button::A))] ? 1 : 0;
        start_presses += raw.buttons[size_t(face_index(Button::Start))] && !prev.buttons[size_t(face_index(Button::Start))];
        dir_changes += raw.left != prev.left || raw.right != prev.right || raw.up != prev.up || raw.down != prev.down;
        opposite += (raw.left && raw.right) || (raw.up && raw.down);
        prev = raw;
    }
    CHECK(a_presses > 600);                         // ten minutes of steady button bursts
    CHECK(start_presses >= 3 && start_presses < 30);  // Start comes rarely
    CHECK(dir_changes > 300 && dir_changes < 3600);   // directions are held, not jittered
    CHECK_EQ(opposite, 0);
}
