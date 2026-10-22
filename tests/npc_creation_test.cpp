#include <cassert>
#include <cstring>
#include <vector>
#include "game/npc_creation.h"
#include "game/layout.h"
#include "xdk/hle.h"
namespace kernel { uint8_t* g_base; }
namespace mem { bool IsCommitted(uint32_t page) { return page>=0x10000 && page<0x400000; } }
void NpcCreationBegin(X86Context&,uint8_t*);
void NpcCreationEnd(X86Context&,uint8_t*);
int main() {
    using namespace game::npc_creation;
    std::vector<uint8_t> memory(0x400000); kernel::g_base=memory.data();
    constexpr uint32_t world=0x10000, stack=0x20000, descriptor=0x30000, transform=0x40000;
    constexpr uint32_t controller=0x50000, actor=0x60000;
    kernel::W32(layout::kWorld,world); kernel::W32(stack+4,3); kernel::W32(stack+20,transform);
    kernel::W64(descriptor+8,0x123456789ABCDEF0ull);
    kernel::W32(controller+0x8C,actor); kernel::W32(actor+0xC4,1);
    float health=100; std::memcpy(memory.data()+actor+0x2F0,&health,4);
    X86Context ctx{}; ctx.esp=stack; ctx.ecx=descriptor; ctx.eax=controller;
    const auto original_memory=memory;
    const auto original_ctx=ctx;
    NpcCreationBegin(ctx,nullptr); NpcCreationEnd(ctx,nullptr);
    assert(memory==original_memory && std::memcmp(&ctx,&original_ctx,sizeof(ctx))==0);
    auto snapshot=State().Get();
    assert(snapshot.attempts==1 && snapshot.created==1 && snapshot.profiles==1 && snapshot.profile_counts[3]==1);
    assert(snapshot.last_resource_readable && snapshot.last_transform_readable);
    NpcCreationBegin(ctx,nullptr); ctx.eax=0; NpcCreationEnd(ctx,nullptr);
    assert(State().Get().empty==1);
    // Uma chamada interna deve encerrar seu próprio registro antes da chamada externa.
    ctx.eax=controller; NpcCreationBegin(ctx,nullptr);
    kernel::W32(stack+4,4); NpcCreationBegin(ctx,nullptr); NpcCreationEnd(ctx,nullptr);
    NpcCreationEnd(ctx,nullptr);
    assert(State().Get().created==3 && State().Get().profiles==2 && State().Get().last_profile==3);
    NpcCreationBegin(ctx,nullptr); State().Reset(); NpcCreationEnd(ctx,nullptr);
    assert(State().Get().attempts==0 && State().Get().created==0);
    // Uma mudança de mundo ou uma pilha ilegível não fornece amostra utilizável.
    NpcCreationBegin(ctx,nullptr); kernel::W32(layout::kWorld,world+0x1000); NpcCreationEnd(ctx,nullptr);
    assert(State().Get().created==0);
    State().World(world+0x1000); assert(State().Get().attempts==0);
    ctx.esp=0xFFFFFFF0; NpcCreationBegin(ctx,nullptr); NpcCreationEnd(ctx,nullptr);
    assert(State().Get().unreadable==1);
    ctx.esp=stack; State().Reset();
    for (int i=0;i<9;++i) NpcCreationBegin(ctx,nullptr);
    for (int i=0;i<9;++i) NpcCreationEnd(ctx,nullptr);
    assert(State().Get().attempts==8 && State().Get().created==8);
    State().Enable(false); NpcCreationBegin(ctx,nullptr); NpcCreationEnd(ctx,nullptr);
    assert(State().Get().attempts==0);
    State().Enable(true); kernel::W32(actor+0xC4,2);
    NpcCreationBegin(ctx,nullptr); NpcCreationEnd(ctx,nullptr);
    assert(State().Get().created==0 && State().Get().unreadable==1);
    kernel::W32(actor+0xC4,1); kernel::W32(stack+4,44);
    NpcCreationBegin(ctx,nullptr); NpcCreationEnd(ctx,nullptr);
    assert(State().Get().created==0 && State().Get().unreadable==2);
}
