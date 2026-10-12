#pragma once

namespace player {

// Turns "is it held down right now?" into seeks: a quick tap jumps a fixed step, and holding
// past a short delay scrubs continuously. Use one per direction and feed it the on-screen
// button and the arrow key together.
class HoldToScrub {
public:
    static constexpr double tap_step = 5.0;      // seconds per tap
    static constexpr double hold_delay = 0.3;    // how long a press must last to count as a hold
    static constexpr double scrub_speed = 10.0;  // seconds of song per second of holding

    explicit HoldToScrub(double direction) : direction_(direction) {}

    // Call once per frame. Returns how many seconds to seek this frame, usually 0.
    double update(bool down, double dt) {
        if (down) {
            held_ += dt;
            return held_ >= hold_delay ? direction_ * scrub_speed * dt : 0.0;
        }

        // Released: it was a tap if it never got as far as scrubbing.
        const bool tapped = held_ > 0.0 && held_ < hold_delay;
        held_ = 0.0;
        return tapped ? direction_ * tap_step : 0.0;
    }

private:
    double direction_;  // -1 back, +1 forward
    double held_ = 0.0;
};
}  // namespace player
