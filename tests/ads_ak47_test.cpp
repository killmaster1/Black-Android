#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include "common/options.h"
#include "core/log.h"
#include "game/ads_ak47.h"
#include "game/ads_ak47_math.h"
#include "game/layout.h"
#include "xdk/hle.h"

// Somente as interfaces externas são substituídas; o código de ADS testado é o da aplicação.
namespace kernel { uint8_t* g_base; }
namespace app { bool menu = false; bool MenuOpen() { return menu; } }
namespace touch { bool look = true; bool LookActive() { return look; } }
namespace logx { void Write(Level, const char*, ...) {} }
namespace game { Options& Opt() { static Options o; return o; } }
using namespace game::ads_ak47;
bool Near(float a, float b) { return std::abs(a-b) < 0.0001f; }
Matrix Identity() { return {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1}; }
int main() {
    Matrix m=Identity(); m[12]=0.3f; m[13]=-0.2f; m[14]=0.5f;
    assert(Adjust(m,0,1,2,3,30,15)==m);
    auto moved=Adjust(m,1,-0.1f,0.2f,0,0,0);
    assert(Near(moved[12],0.2f) && Near(moved[13],0) && Near(moved[14],0.5f));
    auto rotated=Adjust(Identity(),1,0,0,0,90,0);
    assert(Near(rotated[0],0) && Near(rotated[2],-1));
    assert(rotated[3]==0 && rotated[7]==0 && rotated[11]==0 && rotated[15]==1);
    float a=0,b=0;
    for(int i=0;i<30;i++) a=Blend(a,true,1.0f/30);
    for(int i=0;i<60;i++) b=Blend(b,true,1.0f/60);
    assert(Near(a,b) && a>0.99f);
    for(int i=0;i<60;i++) b=Blend(b,false,1.0f/60);
    assert(b<0.0001f);

    std::vector<uint8_t> memory(4*1024*1024);
    kernel::g_base=memory.data();
    constexpr uint32_t world=0x10000, player=world+layout::kWorldPlayer, weapon=0x20000, game=0x30000;
    assert(!RegisterCurrent());
    kernel::W32(layout::kWorld,world); kernel::W32(layout::kGame,game);
    kernel::W32(game+layout::kGameState,game+layout::kFrontEnd);
    kernel::W32(player+0x29c,weapon); kernel::G<uint8_t>(weapon)=3;
    assert(!RegisterCurrent());  // O menu principal não registra armas de uma missão anterior.
    kernel::W32(game+layout::kGameState,0);
    std::memcpy(kernel::HostPtr(player+0x420),m.data(),sizeof(m));
    assert(RegisterCurrent());
    assert(game::Opt().ads_ak47 && game::Opt().ads_ak47_type==3);
    kernel::G<uint8_t>(weapon+0x105)=1;
    BeforePlayerUpdate(player); AfterPlayerUpdate(player);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    BeforePlayerUpdate(player); AfterPlayerUpdate(player);
    Matrix changed;
    std::memcpy(changed.data(),kernel::HostPtr(player+0x420),sizeof(changed));
    assert(changed!=m);
    BeforePlayerUpdate(player);
    std::memcpy(changed.data(),kernel::HostPtr(player+0x420),sizeof(changed));
    assert(changed==m);  // O deslocamento visual não se acumula na matriz do próximo quadro.
    kernel::G<uint8_t>(weapon)=4;
    AfterPlayerUpdate(player);
    std::memcpy(changed.data(),kernel::HostPtr(player+0x420),sizeof(changed));
    assert(changed==m);  // Uma arma diferente não recebe os ajustes da AK47.
    kernel::G<uint8_t>(weapon)=3;
    game::Opt().free_camera=true;
    AfterPlayerUpdate(player);
    std::memcpy(changed.data(),kernel::HostPtr(player+0x420),sizeof(changed));
    assert(changed==m);
    std::puts("ADS AK47: passaram os testes de matriz 3D, transição, registro, restauração e isolamento por arma.");
}
