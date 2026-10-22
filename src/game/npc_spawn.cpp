#include "game/npc_spawn.h"
#include "game/layout.h"
#include "kernel/memory.h"
#include "xdk/hle.h"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
namespace {
bool Readable(uint32_t address,uint32_t size) {
    if (!address || !size || address>std::numeric_limits<uint32_t>::max()-size) return false;
    for (uint32_t page=address & ~uint32_t(0xFFF);;page+=0x1000) {
        if (!mem::IsCommitted(page)) return false;
        if (page==((address+size-1) & ~uint32_t(0xFFF))) return true;
    }
}
bool Actor(uint32_t controller) {
    if (!Readable(controller,0x90)) return false;
    uint32_t actor=kernel::R32(controller+0x8C);
    if (!Readable(actor,0x3A0) || kernel::R32(actor+0xC4)!=1 || kernel::R32(actor+0x384)) return false;
    float health; std::memcpy(&health,kernel::HostPtr(actor+0x2F0),4);
    return std::isfinite(health) && health>0;
}
struct Call {
    bool valid = false;
    uint32_t world = 0, definition = 0, stack = 0;
    uint64_t epoch = 0;
    std::array<uint32_t,11> args{};
};
thread_local std::array<Call,8> calls{};
thread_local uint32_t depth=0;
thread_local bool replaying=false;
}
// 0x1B2640 recebe a definição em ESI e 11 argumentos. A cópia só vive até o retorno dessa chamada.
void NpcSpawnBegin(X86Context& ctx,uint8_t*) {
    uint32_t slot=depth++;
    if (slot>=calls.size()) return;
    auto& call=calls[slot]; call={};
    auto state=game::npc_spawn::State().Get();
    if (replaying || state.status!=game::npc_spawn::Status::Armed || !Readable(ctx.esp,48) || !Readable(ctx.esi,0x60) ||
        ctx.esp<0x4000 || !Readable(ctx.esp-0x4000,0x4000)) return;
    if (static_cast<const uint8_t*>(kernel::HostPtr(ctx.esi))[0x57]>=44) return;
    call.world=kernel::R32(layout::kWorld); call.epoch=state.epoch;
    call.definition=ctx.esi; call.stack=ctx.esp;
    std::memcpy(call.args.data(),kernel::HostPtr(ctx.esp+4),sizeof(call.args));
    call.valid=true;
}
// Os três retornos da função são pareados; só uma criação original bem-sucedida dispara a extra.
void NpcSpawnEnd(X86Context& ctx,uint8_t*) {
    if (!depth) return;
    uint32_t slot=--depth;
    if (slot>=calls.size()) return;
    auto call=calls[slot]; calls[slot]={};
    if (!call.valid || replaying || call.world!=kernel::R32(layout::kWorld) ||
        ctx.esp!=call.stack || !Actor(ctx.eax) || !Readable(call.definition,0x60)) return;
    auto& state=game::npc_spawn::State();
    if (!state.Take(call.world,call.epoch)) return;
    using game::npc_spawn::Status;
    // Exige vaga existente para o primeiro protótipo; não força expansão das reservas.
    uint32_t manager=kernel::R32(0x2D1980), ai=kernel::R32(0x2D1988);
    if (!manager || manager>0xFFFF863F || !Readable(manager+0x79B4,12) ||
        !ai || ai>0xFFFDD31F || !Readable(ai+0x22CDC,4) ||
        kernel::R32(manager+0x79B8)>=kernel::R32(manager+0x79B4) || !kernel::R32(ai+0x22CDC)) {
        state.Finish(call.world,call.epoch,Status::NoRoom); return;
    }
    // CallGuest prepara uma pilha nova abaixo do retorno. Todo o contexto original fica preservado.
    X86Context duplicate=ctx;
    duplicate.esi=call.definition;
    struct ReplayGuard {
        ReplayGuard() { replaying=true; }
        ~ReplayGuard() { replaying=false; }
    } guard;
    uint32_t result=kernel::CallGuest(duplicate,0x1B2640,call.args.data(),call.args.size());
    state.Finish(kernel::R32(layout::kWorld),call.epoch,Actor(result) && result!=ctx.eax ? Status::Created : Status::Failed);
}
