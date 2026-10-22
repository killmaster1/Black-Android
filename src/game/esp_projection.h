#pragma once
#include <array>
#include <cmath>

namespace game::esp_diagnostic {
using Point = std::array<float, 3>;
using Matrix = std::array<float, 16>;
inline bool Finite(const Point& point) {
    for (float value : point) if (!std::isfinite(value)) return false;
    return true;
}
// A matriz de 0xD2F40 produz X, Y e profundidade homogêneos nas três primeiras colunas.
// Após dividir pela profundidade, X e Y estão no intervalo normalizado da imagem [0, 1].
inline bool Project(const Point& point, const Matrix& matrix, float width, float height, float& x, float& y) {
    if (!Finite(point) || !std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0) return false;
    for (int i = 0; i < 16; ++i)
        if (i % 4 != 3 && !std::isfinite(matrix[i])) return false;
    Point projected{};
    for (int column = 0; column < 3; ++column)
        projected[column] = point[0]*matrix[column] + point[1]*matrix[4+column] +
                            point[2]*matrix[8+column] + matrix[12+column];
    if (!Finite(projected) || projected[2] <= 0.001f) return false;
    float u = projected[0]/projected[2], v = projected[1]/projected[2];
    if (!std::isfinite(u) || !std::isfinite(v) || u < 0 || u > 1 || v < 0 || v > 1) return false;
    x = u * width; y = v * height;
    return true;
}
}
