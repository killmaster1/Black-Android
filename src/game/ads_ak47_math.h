#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace game::ads_ak47 {
using Matrix = std::array<float, 16>;

// Interpolação pelo tempo real, sem depender da quantidade de quadros renderizados.
inline float Blend(float current, bool aiming, float dt) {
    if (!std::isfinite(dt)) return current;
    float target = aiming ? 1.0f : 0.0f;
    return target + (current - target) * std::exp(-18.0f * std::clamp(dt, 0.0f, 0.1f));
}

// Ajusta a matriz local da arma. Os componentes homogêneos permanecem intactos.
inline Matrix Adjust(Matrix m, float blend, float x, float y, float z, float yaw, float pitch) {
    constexpr float radians = 3.14159265358979323846f / 180.0f;
    blend = std::clamp(blend, 0.0f, 1.0f);
    float cy = std::cos(yaw * blend * radians), sy = std::sin(yaw * blend * radians);
    float cp = std::cos(pitch * blend * radians), sp = std::sin(pitch * blend * radians);
    for (int row = 0; row < 4; row++) {
        int i = row * 4;
        float a = m[i] * cy + m[i+2] * sy;
        float c = -m[i] * sy + m[i+2] * cy;
        float b = m[i+1] * cp - c * sp;
        c = m[i+1] * sp + c * cp;
        m[i] = a; m[i+1] = b; m[i+2] = c;
    }
    m[12] += x * blend; m[13] += y * blend; m[14] += z * blend;
    return m;
}
}  // namespace game::ads_ak47
