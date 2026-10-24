#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>
#include "game/extreme.h"
#include "common/options.h"
#include "game/layout.h"
#include "xdk/hle.h"
namespace kernel { uint8_t* g_base; }
namespace mem { bool IsCommitted(uint32_t page) { return page>=0x10000 && page<0x400000; } }
namespace game { Options& Opt() { static Options options;return options; } }
void ExtremeBulletDamage(X86Context&,uint8_t*);
int main() {
    using namespace game::extreme;
    HealthScale scale;
    assert(scale.Apply(1,1,100,3)==300);
    assert(scale.Apply(1,1,300,3)==300);
    assert(scale.Apply(1,1,150,1)==50);
    assert(scale.Apply(1,2,100,3)==300);
    scale.Created(1);assert(scale.Apply(1,2,100,3)==300);
    assert(ExtraDamage(100,250)==150 && ExtraDamage(100,400)==300);
    assert(ExtraDamage(300,400)==262.5f && ExtraDamage(800,400)==0);
    std::vector<uint8_t> memory(0x400000);kernel::g_base=memory.data();
    constexpr uint32_t world=0x10000, game_object=0x20000,manager=0x50000,table=0x60000,actor=0x70000;
    kernel::W32(layout::kWorld,world);kernel::W32(layout::kGame,game_object);kernel::W32(0x2D1980,manager);
    kernel::W32(game_object+layout::kGameState,1);kernel::W32(game_object+0x2014C,1);
    kernel::W32(manager+0x79B4,16);kernel::W32(manager+0x79B8,1);kernel::W32(manager+0x79BC,table);
    kernel::W32(table,actor);kernel::W32(actor+0xC4,1);kernel::W32(actor+0x39C,0x80000);
    auto health=[&](float h) { std::memcpy(memory.data()+actor+0x2F0,&h,4); };
    auto current=[&] { float h;std::memcpy(&h,memory.data()+actor+0x2F0,4);return h; };
    health(100);game::Opt().extreme_mode=true;Tick();
    assert(current()==300 && kernel::R32(game_object+0x2014C)==3);
    Tick();assert(current()==300);
    health(150);game::Opt().extreme_mode=false;Tick();
    assert(current()==50 && kernel::R32(game_object+0x2014C)==1);
    X86Context ctx{};ctx.ebx=world+layout::kWorldPlayer;ctx.ebp=0x90000;ctx.xmm[0].f[0]=750;
    float damage=100;std::memcpy(memory.data()+ctx.ebp+0x10,&damage,4);
    game::Opt().extreme_mode=true;ExtremeBulletDamage(ctx,nullptr);assert(ctx.xmm[0].f[0]==600);
    ctx.xmm[0].f[0]=750;game::Opt().god_mode=true;ExtremeBulletDamage(ctx,nullptr);assert(ctx.xmm[0].f[0]==750);
    game::Opt().god_mode=false;ctx.ebx=actor;ExtremeBulletDamage(ctx,nullptr);assert(ctx.xmm[0].f[0]==750);
}
