#pragma once

#include <imgui.h>

#include <cstddef>
#include <span>
#include <vector>

namespace player {

// Bar heights with a little physics, everything in 0..1 of the bar area:
// - a bar jumps up instantly and sinks back at a steady speed, so beats bounce;
// - a peak cap rides on top, hangs for a moment, falls under gravity and hops when it lands.
class BarMeter {
public:
    explicit BarMeter(std::size_t count);

    // `targets` holds where each bar wants to be this frame; `dt` is seconds since the last.
    void update(std::span<const double> targets, double dt);

    std::span<const double> levels() const { return levels_; }
    std::span<const double> peaks() const { return peaks_; }

private:
    std::vector<double> levels_;
    std::vector<double> peaks_;
    std::vector<double> hold_;      // seconds the cap still hangs before falling
    std::vector<double> velocity_;  // cap speed, positive downwards
};

// One band's loudness as a bar target in 0..1: decibels relative to `reference`, tilted by
// `tilt_db_per_octave` around 1 kHz, with -range_db..0 dB filling the bar.
double bar_target(double magnitude, double reference, double centre_hz, double range_db,
                  double tilt_db_per_octave);

// Draw the meter into a rectangle of the current ImGui window: bass on the left in red,
// sweeping through the rainbow to treble on the right in blue.
void draw_bars(const BarMeter& meter, ImVec2 top_left, ImVec2 size);
}  // namespace player
