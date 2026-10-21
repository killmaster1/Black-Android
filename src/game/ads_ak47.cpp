#include "game/ads_ak47.h"
#include "game/ads_ak47_math.h"
#include <atomic>
#include <chrono>
#include <cstring>
#include "common/menu.h"
#include "common/options.h"
#include "core/log.h"
#include "game/layout.h"
#include "host.h"
#include "xdk/hle.h"

namespace game::ads_ak47 {
namespace {
using Clock = std::chrono::steady_clock;
// Evidências: default_us.xbe, SHA1 c5bfed25449c8212d7a976f15246e87ead57713f.
// 0x1E634C lê a arma atual; 0x1E6356 lê seu tipo; 0x1D265D compõe a matriz local com a do jogador.
constexpr uint32_t kWeapon = 0x29C, kLocalMatrix = 0x420, kState = 0xD8, kAiming = 0x105;
uint32_t g_player = 0, g_weapon = 0;
Matrix g_original{}, g_adjusted{};
bool g_patched = false;
std::atomic<bool> g_reset_requested{false};
float g_blend = 0;
Clock::time_point g_last{};

bool Current(uint32_t player) {
    uint32_t world = kernel::R32(layout::kWorld);
    return world && player == world + layout::kWorldPlayer;
}
Matrix Read(uint32_t player) {
    Matrix m;
    std::memcpy(m.data(), kernel::HostPtr(player + kLocalMatrix), sizeof(m));
    return m;
}
void Write(uint32_t player, const Matrix& m) {
    std::memcpy(kernel::HostPtr(player + kLocalMatrix), m.data(), sizeof(m));
}
void Reset() { g_blend = 0; g_weapon = 0; g_last = {}; g_patched = false; }
float Offset(uint32_t value) { return (float(std::min(value, 2000u)) - 1000.0f) / 100.0f; }
float Angle(uint32_t value) { return (float(std::min(value, 2000u)) - 1000.0f) / 10.0f; }
}

bool RegisterCurrent() {
    uint32_t world = kernel::R32(layout::kWorld);
    uint32_t game = kernel::R32(layout::kGame);
    if (!world || !game || kernel::R32(game + layout::kGameState) == game + layout::kFrontEnd) return false;
    uint32_t player = world + layout::kWorldPlayer;
    uint32_t weapon = kernel::R32(player + kWeapon);
    // O jogo usa um índice com sinal de oito bits; valores negativos representam ausência de arma.
    if (!weapon || kernel::G<int8_t>(weapon) < 0) return false;
    Opt().ads_ak47_type = kernel::G<uint8_t>(weapon);
    Opt().ads_ak47 = true;
    // O painel gráfico só solicita a reinicialização; quem altera matrizes é a thread do jogo.
    g_reset_requested = true;
    LOGI("ADS AK47: tipo %u registrado pelo jogador", Opt().ads_ak47_type);
    return true;
}

void BeforePlayerUpdate(uint32_t player) {
    if (g_patched && g_player == player && Current(player) && Read(player) == g_adjusted) Write(player, g_original);
    g_patched = false;
    if (g_reset_requested.exchange(false) || g_player != player) Reset();
    g_player = player;
}

void AfterPlayerUpdate(uint32_t player) {
    if (!Current(player)) { Reset(); return; }
    uint32_t weapon = kernel::R32(player + kWeapon);
    const Options& o = Opt();
    if (!o.ads_ak47 || o.ads_ak47_type >= 128 || !weapon || kernel::G<uint8_t>(weapon) != o.ads_ak47_type ||
        o.free_camera || kernel::R32(player + 0x384)) { Reset(); return; }
    auto now = Clock::now();
    float dt = g_last == Clock::time_point{} ? 0.0f : std::chrono::duration<float>(now - g_last).count();
    if (g_weapon != weapon) { g_blend = 0; dt = 0; }
    g_weapon = weapon;
    g_last = now;
    // 0x1BB385 usa o estado 5 para recarregar. A arma conserva a animação original durante a recarga.
    bool aiming = kernel::G<uint8_t>(weapon + kAiming) && kernel::R32(weapon + kState) != 5;
    if (!touch::LookActive() || app::MenuOpen() || g_focus_paused) return;
    g_blend = Blend(g_blend, aiming, dt);
    if (g_blend < 0.0001f) return;
    g_original = Read(player);
    for (float value : g_original) if (!std::isfinite(value)) { Reset(); return; }
    g_adjusted = Adjust(g_original, g_blend, Offset(o.ads_ak47_x), Offset(o.ads_ak47_y), Offset(o.ads_ak47_z),
                        Angle(o.ads_ak47_yaw), Angle(o.ads_ak47_pitch));
    Write(player, g_adjusted);
    g_player = player;
    g_patched = true;
}
}
