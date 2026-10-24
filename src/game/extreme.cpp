#include "game/extreme.h"
#include "common/options.h"
#include "game/layout.h"
#include "kernel/memory.h"
#include "xdk/hle.h"
#include <cstring>
#include <limits>
namespace game::extreme {
namespace {
uint32_t previous_world=0, difficulty_game=0, previous_difficulty=0;
bool difficulty_patched=false;
bool Readable(uint32_t address,uint32_t size) {
    if (!address || !size || address>std::numeric_limits<uint32_t>::max()-size) return false;
    for (uint32_t page=address & ~uint32_t(0xFFF);;page+=0x1000) {
        if (!mem::IsCommitted(page)) return false;
        if (page==((address+size-1) & ~uint32_t(0xFFF))) return true;
    }
}
float ReadFloat(uint32_t address) { float result;std::memcpy(&result,kernel::HostPtr(address),4);return result; }
}
void Tick() {
    uint32_t world=kernel::R32(layout::kWorld), game=kernel::R32(layout::kGame);
    if (world!=previous_world) { Health().Clear();previous_world=world; }
    if (!game || game>0xFFFDEF8F || !Readable(game+layout::kGameState,4) || !Readable(game+0x2014C,4)) return;
    bool mission=world && kernel::R32(game+layout::kGameState)!=game+layout::kFrontEnd;
    bool enabled=mission && Opt().extreme_mode;
    if (difficulty_patched && (!enabled || difficulty_game!=game)) {
        if (difficulty_game==game && kernel::R32(game+0x2014C)==3) kernel::W32(game+0x2014C,previous_difficulty);
        difficulty_patched=false;
    }
    if (enabled) {
        if (!difficulty_patched) {
            difficulty_game=game;previous_difficulty=kernel::R32(game+0x2014C);difficulty_patched=true;
        }
        kernel::W32(game+0x2014C,3);
    }
    if (!mission || (!enabled && Health().Empty())) return;
    uint32_t manager=kernel::R32(0x2D1980);
    if (!manager || manager>0xFFFF863F || !Readable(manager+0x79B4,12)) return;
    uint32_t capacity=kernel::R32(manager+0x79B4), used=kernel::R32(manager+0x79B8), table=kernel::R32(manager+0x79BC);
    if (!capacity || capacity>128 || used>capacity || !Readable(table,capacity*4)) return;
    std::vector<uint32_t> present;
    for (uint32_t i=0;i<capacity;++i) {
        uint32_t actor=kernel::R32(table+i*4);
        if (!Readable(actor,0x3A0) || kernel::R32(actor+0xC4)!=1 || !kernel::R32(actor+0x39C) || kernel::R32(actor+0x384)) continue;
        present.push_back(actor);
        uint64_t identity=(uint64_t(kernel::R32(actor+0x10))<<32)|kernel::R32(actor+0x324);
        float old=ReadFloat(actor+0x2F0);
        float now=Health().Apply(actor,identity,old,enabled ? Opt().extreme_npc_health : 1);
        if (now!=old && std::isfinite(now)) std::memcpy(kernel::HostPtr(actor+0x2F0),&now,4);
    }
    Health().Prune(present);
}
}
// O jogo já passou pela verificação de invulnerabilidade e carregou a vida em XMM0.
// A subtração original de [EBP+0x10] e o tratamento original de morte continuam sendo executados.
void ExtremeBulletDamage(X86Context& ctx,uint8_t*) {
    if (!game::Opt().extreme_mode || game::Opt().god_mode) return;
    uint32_t world=kernel::R32(layout::kWorld);
    if (!world || ctx.ebx!=world+layout::kWorldPlayer) return;
    float damage;
    std::memcpy(&damage,kernel::HostPtr(ctx.ebp+0x10),4);
    ctx.xmm[0].f[0]-=game::extreme::ExtraDamage(damage,game::Opt().extreme_damage);
}
