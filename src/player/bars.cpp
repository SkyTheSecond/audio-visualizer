#include "bars.hpp"

#include <audioviz/stft.hpp>

#include <algorithm>
#include <cmath>

namespace player {
namespace {
constexpr double fall_speed = 1.2;  // bar heights per second a bar sinks
constexpr double peak_hold = 0.4;   // seconds a cap hangs before it starts to fall
constexpr double gravity = 3.0;     // bar heights per second squared
constexpr double bounce = 0.35;     // fraction of its landing speed a cap keeps, upwards
constexpr double min_bounce = 0.4;  // slower landings than this just stop
}  // namespace

BarMeter::BarMeter(std::size_t count)
    : levels_(count, 0.0), peaks_(count, 0.0), hold_(count, 0.0), velocity_(count, 0.0) {}

void BarMeter::update(std::span<const double> targets, double dt) {
    for (std::size_t i = 0; i < levels_.size() && i < targets.size(); i++) {
        // Instant attack, steady release.
        const double target = std::clamp(targets[i], 0.0, 1.0);
        levels_[i] = std::max(target, levels_[i] - fall_speed * dt);

        if (levels_[i] >= peaks_[i]) {
            // Pushed up by its bar: stick to the top and start hanging.
            peaks_[i] = levels_[i];
            hold_[i] = peak_hold;
            velocity_[i] = 0.0;
        } else if (hold_[i] > 0.0) {
            hold_[i] -= dt;
        } else {
            velocity_[i] += gravity * dt;
            peaks_[i] -= velocity_[i] * dt;

            if (peaks_[i] <= levels_[i]) {
                // Landed on the bar: hop back up a little if it came down fast enough.
                peaks_[i] = levels_[i];
                velocity_[i] = velocity_[i] > min_bounce ? -bounce * velocity_[i] : 0.0;
            }
        }
    }
}

double bar_target(double magnitude, double reference, double centre_hz, double range_db,
                  double tilt_db_per_octave) {
    // Tilt before clamping, so silence stays at zero in every band.
    const double db = audioviz::to_decibels(magnitude, reference, -200.0) +
                      tilt_db_per_octave * std::log2(centre_hz / 1000.0);
    return std::clamp((db + range_db) / range_db, 0.0, 1.0);
}

void draw_bars(const BarMeter& meter, ImVec2 top_left, ImVec2 size) {
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const std::span<const double> levels = meter.levels();
    const std::span<const double> peaks = meter.peaks();
    if (levels.empty() || size.x <= 0.0f || size.y <= 0.0f) return;

    const float slot = size.x / static_cast<float>(levels.size());
    const float gap = std::max(1.0f, slot * 0.2f);
    const float bottom = top_left.y + size.y;
    const float cap_height = std::max(2.0f, size.y * 0.008f);

    for (std::size_t i = 0; i < levels.size(); i++) {
        // Hue 0 is red; 0.66 is blue, with orange, yellow, green and cyan on the way.
        const float hue = 0.66f * static_cast<float>(i) / static_cast<float>(levels.size() - 1);
        const ImU32 top_color = ImColor::HSV(hue, 0.8f, 1.0f);
        const ImU32 bottom_color = ImColor::HSV(hue, 0.9f, 0.35f);
        const ImU32 cap_color = ImColor::HSV(hue, 0.25f, 1.0f);

        const float left = top_left.x + static_cast<float>(i) * slot + gap / 2.0f;
        const float right = left + slot - gap;
        const float top = bottom - static_cast<float>(levels[i]) * size.y;
        const float cap = bottom - static_cast<float>(peaks[i]) * size.y;

        // Corner colours go top-left, top-right, bottom-right, bottom-left.
        draw->AddRectFilledMultiColor(ImVec2(left, top), ImVec2(right, bottom), top_color,
                                      top_color, bottom_color, bottom_color);
        draw->AddRectFilled(ImVec2(left, cap - cap_height), ImVec2(right, cap), cap_color);
    }
}
}  // namespace player
