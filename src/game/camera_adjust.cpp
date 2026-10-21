#include "game/camera_adjust.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include "common/menu.h"
#include "common/options.h"
#include "game/layout.h"
#include "host.h"
#include "xdk/hle.h"

namespace game::camera_adjust {
namespace {
std::mutex g_lock;
Parameters g_parameters;
std::atomic<bool> g_enabled{false};
using Point = std::array<float, 3>;
uint32_t g_player = 0;
Point g_real{}, g_modified{};
bool g_patched = false;
constexpr uint32_t kEye = 0x100;
bool Current(uint32_t player) {
    uint32_t world = kernel::R32(layout::kWorld);
    return world && player == world + layout::kWorldPlayer;
}
Point Read(uint32_t address) { Point p; std::memcpy(p.data(), kernel::HostPtr(address), sizeof(p)); return p; }
void Write(uint32_t player, const Point& p) { std::memcpy(kernel::HostPtr(player + kEye), p.data(), sizeof(p)); }
float Safe(float v) { return std::isfinite(v) ? std::clamp(v, -2.0f, 2.0f) : 0; }
float Decode(uint32_t v) { return (float(std::clamp(v, 800u, 1200u)) - 1000) / 100; }
uint32_t Encode(float v) { return uint32_t(std::lround(Safe(v) * 100) + 1000); }
}
Parameters Get() { std::lock_guard lock(g_lock); return g_parameters; }
void Set(Parameters p) {
    p.side = Safe(p.side); p.height = Safe(p.height); p.distance = Safe(p.distance);
    p.fov = std::clamp(p.fov, 45, 140);
    std::lock_guard lock(g_lock); g_parameters = p;
}
bool Enabled() { return g_enabled.load(); }
void Enable(bool enabled) { g_enabled = enabled; }
void LoadPreferences() {
    const auto& o = Opt();
    Set({Decode(o.camera_side), Decode(o.camera_height), Decode(o.camera_distance), int(std::min(o.camera_fov, 140u))});
}
void StorePreferences() {
    auto p = Get(); auto& o = Opt();
    o.camera_side = Encode(p.side); o.camera_height = Encode(p.height); o.camera_distance = Encode(p.distance);
    o.camera_fov = uint32_t(p.fov);
}
uint32_t Fov(uint32_t original) { return Enabled() ? uint32_t(Get().fov) : original; }
void BeforePlayerUpdate(uint32_t player) {
    if (g_patched && player == g_player && Current(player) && Read(player + kEye) == g_modified) Write(player, g_real);
    g_patched = false;
    g_player = player;
}
void View(uint32_t player) {
    if (!Current(player)) return;
    bool active = Enabled() && !Opt().free_camera && !app::MenuOpen() && !g_focus_paused &&
                  touch::LookActive() && !kernel::R32(player + 0x384);
    if (!active) {
        if (g_patched && player == g_player && Read(player + kEye) == g_modified) Write(player, g_real);
        g_patched = false;
        return;
    }
    // A matriz do jogador começa em +0xD0; seus eixos são seguidos pelos olhos em +0x100.
    Point eye = Read(player + kEye);
    if (g_patched && player == g_player && eye == g_modified) eye = g_real;
    auto p = Get();
    const float offsets[] = {p.side, p.height, p.distance};
    Point result = eye;
    for (int axis = 0; axis < 3; axis++) {
        auto basis = Read(player + 0xD0 + uint32_t(axis) * 16);
        for (float value : basis) if (!std::isfinite(value)) return;
        for (int i = 0; i < 3; i++) result[i] += basis[i] * offsets[axis];
    }
    for (float value : result) if (!std::isfinite(value)) return;
    g_real = eye; g_modified = result; g_player = player;
    Write(player, result);
    g_patched = true;
}
}
