#include "game/npc_creation.h"
#include "game/layout.h"
#include "kernel/memory.h"
#include "xdk/hle.h"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
bool Readable(uint32_t address, uint32_t size) {
    if (!address || !size || address>std::numeric_limits<uint32_t>::max()-size) return false;
    for (uint32_t page=address & ~uint32_t(0xFFF);;page+=0x1000) {
        if (!mem::IsCommitted(page)) return false;
        if (page==((address+size-1) & ~uint32_t(0xFFF))) return true;
    }
}
struct Pending {
    uint32_t world = 0;
    uint64_t token = 0;
    game::npc_creation::Sample sample;
    bool stack_readable = false;
};
// As chamadas aninhadas de uma mesma thread são pareadas sem guardar ponteiros para uso futuro.
thread_local std::array<Pending,8> pending{};
thread_local uint32_t depth = 0;
}

// Entrada original de 0x1AF880: 12 argumentos na pilha, além de ECX e EDI.
void NpcCreationBegin(X86Context& ctx, uint8_t*) {
    uint32_t slot=depth++;
    if (slot>=pending.size()) return;
    auto& call=pending[slot]; call={};
    auto& state=game::npc_creation::State();
    if (!state.Enabled()) return;
    call.world=kernel::R32(layout::kWorld);
    call.token=state.Begin(call.world);
    if (!call.token || !Readable(ctx.esp,52)) return;
    call.stack_readable=true;
    call.sample.profile=kernel::R32(ctx.esp+4);
    if (ctx.ecx<=0xFFFFFFEF && Readable(ctx.ecx+8,8)) {
        call.sample.resource=kernel::R64(ctx.ecx+8);
        call.sample.resource_readable=true;
    }
    uint32_t transform=kernel::R32(ctx.esp+20);
    call.sample.transform_readable=transform<=0xFFFFFFDF && Readable(transform+16,16);
}

// Ambos os retornos preservam EAX: zero, ou o controlador cuja ligação ao personagem fica em +0x8C.
void NpcCreationEnd(X86Context& ctx, uint8_t*) {
    if (!depth) return;
    uint32_t slot=--depth;
    if (slot>=pending.size()) return;
    auto call=pending[slot]; pending[slot]={};
    if (!call.token) return;
    using game::npc_creation::Result;
    Result result=ctx.eax ? Result::Unreadable : Result::Empty;
    if (call.stack_readable && ctx.eax && ctx.eax<=0xFFFFFF6F && Readable(ctx.eax+0x8C,4)) {
        uint32_t actor=kernel::R32(ctx.eax+0x8C);
        if (Readable(actor,0x3A0) && kernel::R32(actor+0xC4)==1) {
            float health;
            std::memcpy(&health,kernel::HostPtr(actor+0x2F0),sizeof(health));
            if (std::isfinite(health) && health>0) result=Result::Created;
        }
    }
    if (!call.stack_readable) result=Result::Unreadable;
    game::npc_creation::State().Finish(kernel::R32(layout::kWorld),call.token,call.sample,result);
}
