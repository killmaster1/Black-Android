#include <cassert>
#include <cstring>
#include <vector>
#include "game/npc_spawn.h"
#include "game/layout.h"
#include "xdk/hle.h"
namespace kernel { uint8_t* g_base; }
namespace mem { bool IsCommitted(uint32_t page) { return page>=0x10000 && page<0x400000; } }
void NpcSpawnBegin(X86Context&,uint8_t*);
void NpcSpawnEnd(X86Context&,uint8_t*);
static int guest_calls=0;
static bool fail_extra=false;
static constexpr uint32_t kExtra=0x70000;
namespace kernel {
uint32_t CallGuest(X86Context& ctx,uint32_t function,const uint32_t* args,size_t count) {
    assert(function==0x1B2640 && count==11 && args[0]==11 && args[10]==21 && ctx.esi==0x30000);
    ++guest_calls;
    // A reentrada no hook não deve gerar uma terceira chamada.
    NpcSpawnBegin(ctx,nullptr); ctx.eax=kExtra; NpcSpawnEnd(ctx,nullptr);
    ctx.esi=0;ctx.ebp=0;ctx.esp=0;
    return fail_extra ? 0 : kExtra;
}
}
int main() {
    using namespace game::npc_spawn;
    std::vector<uint8_t> memory(0x400000);kernel::g_base=memory.data();
    constexpr uint32_t world=0x10000,stack=0x20000,definition=0x30000,manager=0x40000,ai=0x50000;
    constexpr uint32_t original=0x60000,actor=0x80000,extra_actor=0x90000;
    kernel::W32(layout::kWorld,world);kernel::W32(0x2D1980,manager);kernel::W32(0x2D1988,ai);
    kernel::W32(manager+0x79B4,16);kernel::W32(manager+0x79B8,2);kernel::W32(ai+0x22CDC,kExtra);
    kernel::W32(original+0x8C,actor);kernel::W32(kExtra+0x8C,extra_actor);
    float health=100;
    for (auto target : {actor,extra_actor}) {
        kernel::W32(target+0xC4,1);std::memcpy(memory.data()+target+0x2F0,&health,4);
    }
    memory[definition+0x57]=36;
    for (uint32_t i=0;i<11;++i) kernel::W32(stack+4+i*4,11+i);
    X86Context ctx{};ctx.esi=definition;ctx.esp=stack;ctx.eax=original;ctx.ebp=0x1234;
    State().World(world);
    NpcSpawnBegin(ctx,nullptr);NpcSpawnEnd(ctx,nullptr);assert(guest_calls==0);
    assert(State().Arm());NpcSpawnBegin(ctx,nullptr);ctx.eax=0;NpcSpawnEnd(ctx,nullptr);
    assert(State().Get().status==Status::Armed && guest_calls==0);
    ctx.eax=original;
    const auto saved_ctx=ctx;const auto saved_memory=memory;
    NpcSpawnBegin(ctx,nullptr);NpcSpawnEnd(ctx,nullptr);
    assert(guest_calls==1 && State().Get().created==1 && State().Get().status==Status::Created);
    assert(std::memcmp(&ctx,&saved_ctx,sizeof(ctx))==0 && memory==saved_memory);
    NpcSpawnBegin(ctx,nullptr);NpcSpawnEnd(ctx,nullptr);assert(guest_calls==1);
    assert(State().Arm());NpcSpawnBegin(ctx,nullptr);State().Cancel();NpcSpawnEnd(ctx,nullptr);
    assert(guest_calls==1);
    assert(State().Arm());NpcSpawnBegin(ctx,nullptr);
    kernel::W32(manager+0x79B8,16);NpcSpawnEnd(ctx,nullptr);
    assert(guest_calls==1 && State().Get().status==Status::NoRoom);
    kernel::W32(manager+0x79B8,2);
    fail_extra=true;
    assert(State().Arm());NpcSpawnBegin(ctx,nullptr);NpcSpawnEnd(ctx,nullptr);
    assert(guest_calls==2 && State().Get().created==1 && State().Get().status==Status::Failed);
    assert(std::memcmp(&ctx,&saved_ctx,sizeof(ctx))==0);
    assert(State().Arm());NpcSpawnBegin(ctx,nullptr);kernel::W32(layout::kWorld,world+1);NpcSpawnEnd(ctx,nullptr);
    State().World(world+1);assert(State().Get().status==Status::Idle && guest_calls==2);
    kernel::W32(layout::kWorld,world);State().World(world);
    assert(State().Arm());ctx.esp=0xFFFFFFF0;NpcSpawnBegin(ctx,nullptr);NpcSpawnEnd(ctx,nullptr);
    assert(guest_calls==2 && State().Get().status==Status::Armed);
    auto previous_epoch=State().Get().epoch;
    State().Reset();
    assert(State().Get().status==Status::Idle && State().Get().created==0 && State().Get().epoch!=previous_epoch);
}
