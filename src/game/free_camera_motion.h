#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace game::free_camera {

using Point = std::array<float, 3>;

inline bool Finite(const Point& p) {
    return std::isfinite(p[0]) && std::isfinite(p[1]) && std::isfinite(p[2]);
}

// Xbox view coordinates: y up. Forward includes pitch; strafing stays horizontal.
inline void Move(Point& position, Point forward, float right, float ahead, float up, float speed, float dt) {
    if (!Finite(position) || !Finite(forward) || !std::isfinite(dt) || !std::isfinite(speed)) return;
    float length = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
    if (length < 0.0001f) return;
    for (float& v : forward) v /= length;
    float horizontal = std::hypot(forward[0], forward[2]);
    Point strafe = horizontal > 0.0001f ? Point{forward[2] / horizontal, 0, -forward[0] / horizontal}
                                       : Point{1, 0, 0};
    Point motion{forward[0] * ahead + strafe[0] * right, forward[1] * ahead + up,
                 forward[2] * ahead + strafe[2] * right};
    float magnitude = std::sqrt(motion[0] * motion[0] + motion[1] * motion[1] + motion[2] * motion[2]);
    float step = std::clamp(speed, 10.0f, 2000.0f) * std::clamp(dt, 0.0f, 0.1f) / std::max(magnitude, 1.0f);
    for (int i = 0; i < 3; i++) position[i] += motion[i] * step;
}

inline float Stick(int value) {
    float v = std::clamp(float(value) / 32767.0f, -1.0f, 1.0f);
    constexpr float deadzone = 0.15f;
    return std::abs(v) <= deadzone ? 0.0f : std::copysign((std::abs(v) - deadzone) / (1 - deadzone), v);
}

}  // namespace game::free_camera
