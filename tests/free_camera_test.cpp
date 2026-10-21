#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <vector>

#include "common/options.h"
#include "core/input.h"
#include "core/log.h"
#include "game/free_camera.h"
#include "game/free_camera_motion.h"
#include "game/layout.h"
#include "xdk/hle.h"

// host boundaries only: exercise the real camera implementation against a simulated guest address space
namespace kernel { uint8_t* g_base; }
namespace app { bool menu = false; bool MenuOpen() { return menu; } }
namespace touch { bool look = true; bool LookActive() { return look; } }
namespace logx { void Write(Level, const char*, ...) {} }
namespace game { Options& Opt() { static Options o; return o; } }

using namespace game::free_camera;

bool Near(float a, float b) { return std::abs(a - b) < 0.001f; }
void Eye(uint32_t player, Point p) { std::memcpy(kernel::HostPtr(player + 0x100), p.data(), sizeof(p)); }
Point Eye(uint32_t player) {
    Point p;
    std::memcpy(p.data(), kernel::HostPtr(player + 0x100), sizeof(p));
    return p;
}

int main() {
    Point forward{0, 0, 1}, at{};
    Move(at, forward, 0, 1, 0, 200, 0.05f);
    assert(Near(at[2], 10));
    at = {};
    Move(at, forward, 1, 1, 1, 200, 0.05f);
    assert(Near(std::sqrt(at[0]*at[0]+at[1]*at[1]+at[2]*at[2]), 10));
    assert(at[0] > 0 && at[1] > 0 && at[2] > 0);
    Point fast{}, slow{};
    for (int i = 0; i < 60; i++) Move(fast, forward, 0, 1, 0, 200, 1.0f/60);
    for (int i = 0; i < 30; i++) Move(slow, forward, 0, 1, 0, 200, 1.0f/30);
    assert(Near(fast[2], slow[2]));
    at = {};
    Move(at, forward, 0, 1, 0, 200, 60);
    assert(Near(at[2], 20));  // no jump after a long pause
    Point invalid{std::numeric_limits<float>::quiet_NaN(), 0, 0};
    Move(at, invalid, 0, 1, 0, 200, 1);
    assert(Near(at[2], 20));
    assert(Stick(1000) == 0 && Stick(-1000) == 0);
    assert(Stick(32767) == 1 && Stick(-32768) == -1);

    std::vector<uint8_t> memory(4*1024*1024);
    kernel::g_base = memory.data();
    constexpr uint32_t world = 0x10000, player = world + layout::kWorldPlayer;
    kernel::W32(layout::kWorld, world);
    Eye(player, {10, 20, 30});
    std::memcpy(kernel::HostPtr(player + 0xF0), forward.data(), sizeof(forward));
    input::Pad pad{.connected = true};
    pad.lx = 32767; pad.ly = 32767; pad.rx = 12345; pad.ry = -12345;
    pad.analog[7] = 255; pad.analog[0] = 255;
    pad.buttons = 0xFFFF;
    auto original = pad;
    FilterPad(pad);
    assert(pad.lx == original.lx && pad.analog[7] == 255);  // disabled is a pass-through
    game::Opt().free_camera = true;
    FilterPad(pad);
    assert(pad.connected && pad.lx == 0 && pad.ly == 0);
    assert(pad.rx == original.rx && pad.ry == original.ry);
    for (uint8_t b : pad.analog) assert(b == 0);
    assert(pad.buttons == ((1u<<4)|(1u<<5)));  // menus remain reachable
    BeforePlayerUpdate(player);
    View(player);
    assert(Active() && Eye(player) == (Point{10, 20, 30}));
    // the game writes a new physical eye later in its update; the detached view must remain fixed
    Eye(player, {40, 50, 60});
    AfterPlayerUpdate(player);
    assert(Eye(player) == (Point{10, 20, 30}));
    BeforePlayerUpdate(player);
    assert(Eye(player) == (Point{40, 50, 60}));
    app::menu = true;
    AfterPlayerUpdate(player);
    assert(Eye(player) == (Point{40, 50, 60}));  // no camera override in menus
    app::menu = false;
    game::Opt().free_camera = false;
    AfterPlayerUpdate(player);
    assert(!Active() && Eye(player) == (Point{40, 50, 60}));

    // a new world must never receive an old player's saved eye
    game::Opt().free_camera = true;
    FilterPad(original);
    BeforePlayerUpdate(player);
    AfterPlayerUpdate(player);
    Eye(player, {70, 80, 90});
    AfterPlayerUpdate(player);
    assert(Eye(player) == (Point{40, 50, 60}));
    game::Opt().free_camera = false;
    View(player);
    assert(!Active() && Eye(player) == (Point{70, 80, 90}));  // disable between update hooks
    game::Opt().free_camera = true;
    FilterPad(original);
    BeforePlayerUpdate(player);
    AfterPlayerUpdate(player);
    kernel::W32(player + 0x384, 1);
    View(player);
    assert(!Active() && Eye(player) == (Point{70, 80, 90}));  // death stops overriding the view
    kernel::W32(player + 0x384, 0);
    BeforePlayerUpdate(player);
    AfterPlayerUpdate(player);
    kernel::W32(layout::kWorld, world + 0x1000);
    constexpr uint32_t next = player + 0x1000;
    Eye(next, {100, 200, 300});
    BeforePlayerUpdate(next);
    assert(Eye(next) == (Point{100, 200, 300}));
    assert(!Active());
    std::puts("free camera: movement, frame rate, input isolation, eye restoration, menus and world transition passed");
}
