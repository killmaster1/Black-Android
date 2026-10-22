#pragma once
#include <array>
#include <algorithm>
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
struct Box { float left = 0, top = 0, right = 0, bottom = 0; };
// Retângulo aproximado a partir dos pés e de uma altura configurável no eixo Y do mundo.
inline bool ProjectBox(Point feet, float height, const Matrix& matrix, float width, float screen_height, Box& box) {
    if (!std::isfinite(height) || height <= 0) return false;
    Point head = feet; head[1] += height;
    float fx, fy, hx, hy;
    if (!Project(feet,matrix,width,screen_height,fx,fy) || !Project(head,matrix,width,screen_height,hx,hy)) return false;
    const float top = std::min(fy,hy), bottom = std::max(fy,hy);
    const float projected_height = bottom-top;
    if (projected_height < 2) return false;
    const float half_width = projected_height*0.22f;
    const float center = (fx+hx)*0.5f;
    box = {std::max(0.0f,center-half_width), top, std::min(width,center+half_width), bottom};
    return box.right > box.left;
}

// Direção na borda da imagem; mantém o lado correto mesmo para alvos atrás da câmera.
inline bool OffscreenDirection(const Point& point, const Matrix& matrix, float width, float height, float& x, float& y) {
    float sx,sy;
    if (Project(point,matrix,width,height,sx,sy)) return false;
    if (!Finite(point) || !std::isfinite(width) || !std::isfinite(height) || width<64 || height<64) return false;
    for (int i=0;i<16;++i) if (i%4!=3 && !std::isfinite(matrix[i])) return false;
    Point projected{};
    for (int c=0;c<3;++c)
        projected[c]=point[0]*matrix[c]+point[1]*matrix[4+c]+point[2]*matrix[8+c]+matrix[12+c];
    if (!Finite(projected)) return false;
    float dx=(projected[0]-0.5f*projected[2])*width;
    float dy=(projected[1]-0.5f*projected[2])*height;
    if (std::hypot(dx,dy)<0.001f) {
        if (std::abs(projected[2])<0.001f) return false;
        dy=1; // Diretamente atrás: indica a borda inferior.
    }
    const float rx=width*0.5f-24, ry=height*0.5f-24;
    const float scale=1/std::max(std::abs(dx)/rx,std::abs(dy)/ry);
    x=width*0.5f+dx*scale; y=height*0.5f+dy*scale;
    return std::isfinite(x) && std::isfinite(y);
}

}
