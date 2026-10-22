#include <cassert>
#include <cstring>
#include <vector>
#include "game/esp_diagnostic.h"
#include "game/layout.h"
#include "xdk/hle.h"
namespace kernel { uint8_t* g_base; }
namespace mem { bool IsCommitted(uint32_t page) { return page >= 0x10000 && page < 0x400000; } }
void EspObserveCandidate(X86Context&, uint8_t*);
void EspCaptureProjection(X86Context&, uint8_t*);
int main() {
    std::vector<uint8_t> memory(0x400000);
    kernel::g_base=memory.data();
    auto& state=game::esp_diagnostic::State();
    state.Enable(true); state.Tick(0x10000,1);
    X86Context ctx{};
    ctx.edx=0x20000;
    game::esp_diagnostic::Point position{15,20,30};
    std::memcpy(memory.data()+ctx.edx+0xA0,position.data(),sizeof(position));
    const auto before=memory;
    EspObserveCandidate(ctx,nullptr);
    assert(state.Get().point_count==1 && state.Get().points[0].position==position);
    assert(memory==before && ctx.edx==0x20000);
    // Nenhuma leitura posterior do personagem: o diagnóstico já possui uma cópia.
    std::memset(memory.data()+ctx.edx+0xA0,0,sizeof(position));
    assert(state.Get().points[0].position==position);
    ctx.edx=0xFFFFFFFC; EspObserveCandidate(ctx,nullptr);
    assert(state.Get().point_count==1);
    constexpr uint32_t display=0x30000;
    kernel::W32(layout::kDisplay,display);
    ctx.edi=display+0xD450;
    game::esp_diagnostic::Matrix matrix{};
    matrix[0]=0.5f; matrix[5]=-0.5f; matrix[8]=matrix[9]=0.5f; matrix[10]=1;
    std::memcpy(memory.data()+ctx.edi,matrix.data(),sizeof(matrix));
    const auto projection_before=memory;
    EspCaptureProjection(ctx,nullptr);
    assert(state.Get().camera_valid && state.Get().camera==matrix);
    assert(memory==projection_before);
    // A lista atualiza NPCs parados e em movimento sem uma nova consulta da mira.
    constexpr uint32_t manager=0x50000, table=0x60000, npc=0x70000;
    kernel::W32(0x2D1980,manager);
    kernel::W32(manager+0x79B4,1); kernel::W32(manager+0x79B8,32); kernel::W32(manager+0x79BC,table);
    kernel::W32(table,npc); kernel::W32(npc+0xC4,1); kernel::W32(npc+0x39C,0x80000);
    float health=100; std::memcpy(memory.data()+npc+0x2F0,&health,4);
    std::memcpy(memory.data()+npc+0xA0,position.data(),sizeof(position));
    const auto scan_before=memory;
    auto observations=state.Get().observations;
    EspCaptureProjection(ctx,nullptr);
    auto snapshot=state.Get();
    assert(snapshot.live.valid && snapshot.live.count==1 && snapshot.live.points[0].position==position);
    assert(memory==scan_before);
    state.Tick(0x10000,150); EspCaptureProjection(ctx,nullptr);
    assert(state.Get().live.valid && state.Get().live.count==1 && state.Get().observations==observations);
    position[0]+=10; std::memcpy(memory.data()+npc+0xA0,position.data(),sizeof(position));
    EspCaptureProjection(ctx,nullptr);
    assert(state.Get().live.points[0].position==position);
    kernel::W32(npc+0x384,1); EspCaptureProjection(ctx,nullptr);
    assert(state.Get().live.valid && state.Get().live.count==0);
    kernel::W32(npc+0x384,0); health=0; std::memcpy(memory.data()+npc+0x2F0,&health,4);
    EspCaptureProjection(ctx,nullptr); assert(state.Get().live.count==0);
    health=100; std::memcpy(memory.data()+npc+0x2F0,&health,4);
    kernel::W32(npc+0xC4,2); EspCaptureProjection(ctx,nullptr);
    assert(state.Get().live.count==0);
    kernel::W32(manager+0x79B4,33); EspCaptureProjection(ctx,nullptr);
    assert(!state.Get().live.valid);
    kernel::W32(manager+0x79B4,1); kernel::W32(table,0xFFFFFFFC);
    EspCaptureProjection(ctx,nullptr); assert(!state.Get().live.valid);
    kernel::W32(manager+0x79B4,0); EspCaptureProjection(ctx,nullptr);
    assert(state.Get().live.valid && state.Get().live.count==0);
    state.Tick(0x10000,153); assert(!state.Get().live.valid);
    state.Reset(); state.Tick(0x10000,154);
    ctx.edi=0xFFFFFFFC; EspCaptureProjection(ctx,nullptr);
    assert(!state.Get().camera_valid);
}
