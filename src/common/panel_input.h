#pragma once
#include <cstdint>
#include <unordered_set>

namespace camera_tools {
struct Rect {
    float x = 0, y = 0, width = 0, height = 0;
    bool Contains(float px, float py) const { return width > 0 && height > 0 && px >= x && py >= y && px < x+width && py < y+height; }
};
// O destino é decidido no início do gesto e mantido até soltar, mesmo fora do painel.
class PanelInput {
    std::unordered_set<int64_t> fingers;
    uint32_t buttons = 0;
public:
    bool FingerDown(int64_t id, bool inside) { if (inside) fingers.insert(id); return inside; }
    bool FingerMove(int64_t id) const { return fingers.count(id) != 0; }
    bool FingerUp(int64_t id) { return fingers.erase(id) != 0; }
    bool MouseDown(unsigned button, bool inside) {
        if (inside && button < 32) buttons |= 1u << button;
        return inside;
    }
    bool MouseUp(unsigned button) {
        if (button >= 32) return false;
        bool ours = buttons & (1u << button);
        buttons &= ~(1u << button);
        return ours;
    }
    bool MouseMove(bool inside) const { return inside || buttons != 0; }
    void Clear() { fingers.clear(); buttons = 0; }
};
}
