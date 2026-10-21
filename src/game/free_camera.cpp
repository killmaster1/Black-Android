#include "game/free_camera.h"
#include "game/free_camera_motion.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>

#include "app/app.h"
#include "common/menu.h"
#include "common/options.h"
#include "core/input.h"
#include "core/log.h"
#include "game/layout.h"
#include "host.h"
#include "xdk/hle.h"

namespace game::free_camera {
namespace {
using Clock = std::chrono::steady_clock;
constexpr uint32_t kEye = 0x100;
constexpr uint32_t kForward = 0xF0;
constexpr uint32_t kDying = 0x384;

struct Controls {
    float right = 0, ahead = 0, up = 0;
    bool play = false;
    Clock::time_point read{};
};
std::mutex g_input_lock;
Controls g_input;
std::atomic<bool> g_active{false};
std::atomic<unsigned> g_toggle_requests{0};

// Só a thread do jogo acessa a memória emulada e a posição da câmera.
uint32_t g_player = 0;
Point g_position{}, g_real_eye{};
bool g_patched = false;
bool g_stepped = false;
Clock::time_point g_last{};

Point Read(uint32_t address) {
    Point p;
    std::memcpy(p.data(), kernel::HostPtr(address), sizeof(p));
    return p;
}
void WriteEye(uint32_t player, const Point& p) {
    std::memcpy(kernel::HostPtr(player + kEye), p.data(), sizeof(p));
}
bool Current(uint32_t player) {
    uint32_t world = kernel::R32(layout::kWorld);
    return world && player == world + layout::kWorldPlayer;
}
void Reset() {
    if (g_active.exchange(false)) LOGI("câmera livre: desligada");
    g_player = 0;
    g_patched = false;
    g_last = {};
}
}  // namespace

bool Active() { return g_active.load(); }

void Toggle() {
    Opt().free_camera = !Opt().free_camera;
    if (!Opt().free_camera) {
        if (g_patched && Current(g_player)) WriteEye(g_player, g_real_eye);
        Reset();
    }
}

void RequestToggle() { g_toggle_requests.fetch_add(1); }
void ProcessRequests() { if (g_toggle_requests.exchange(0) & 1u) Toggle(); }

void FilterPad(input::Pad& p) {
    bool play = Opt().free_camera && touch::LookActive() && !app::MenuOpen();
    {
        std::lock_guard lock(g_input_lock);
        g_input = {Stick(p.lx), Stick(p.ly), float(int(p.analog[7]) - int(p.analog[6])) / 255.0f,
                   play, Clock::now()};
    }
    if (!play) return;
    // O analógico direito continua girando a visão do jogo; toque e mouse seguem o caminho normal.
    p.lx = p.ly = 0;
    for (uint8_t& button : p.analog) button = 0;
    p.buttons &= (1u << 4) | (1u << 5);  // Start e Back continuam abrindo os menus.
}

void BeforePlayerUpdate(uint32_t player) {
    ProcessRequests();
    if (!Current(player)) { Reset(); return; }
    if (g_patched && g_player == player) WriteEye(player, g_real_eye);
    g_patched = false;
    g_stepped = false;
    if (g_player && g_player != player) Reset();
}

void View(uint32_t player) {
    if (!Current(player)) return;
    Controls controls;
    {
        std::lock_guard lock(g_input_lock);
        controls = g_input;
    }
    auto now = Clock::now();
    bool enabled = Opt().free_camera && !kernel::R32(player + kDying);
    bool play = enabled && controls.play && !app::MenuOpen() && !g_focus_paused &&
                now - controls.read < std::chrono::milliseconds(250);
    if (!play) {
        if (g_patched && g_player == player) WriteEye(player, g_real_eye);
        g_patched = false;
        // A pausa preserva a posição da câmera, sem acumular tempo de movimento.
        if (!enabled) Reset();
        else g_last = now;
        return;
    }
    Point eye = Read(player + kEye);
    Point forward = Read(player + kForward);
    if (!Finite(eye) || !Finite(forward)) { Reset(); return; }
    if (!Active() || g_player != player) {
        g_player = player;
        g_position = eye;
        g_last = now;
        g_active = true;
        LOGI("câmera livre: ligada, jogador=%08X olhos=(%.2f, %.2f, %.2f)", player, eye[0], eye[1], eye[2]);
    }
    if (!g_stepped) {
        float dt = std::chrono::duration<float>(now - g_last).count();
        Move(g_position, forward, controls.right, controls.ahead, controls.up, float(Opt().free_camera_speed), dt);
        g_last = now;
        g_stepped = true;
    }
    // FieldOfView e o fim da atualização do jogador podem passar aqui no mesmo quadro.
    if (!g_patched || eye != g_position) g_real_eye = eye;
    WriteEye(player, g_position);
    g_patched = true;
}

void AfterPlayerUpdate(uint32_t player) { View(player); }

}  // namespace game::free_camera
