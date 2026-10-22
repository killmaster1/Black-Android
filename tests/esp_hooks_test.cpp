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
    state.Reset(); state.Tick(0x10000,2);
    ctx.edi=0xFFFFFFFC; EspCaptureProjection(ctx,nullptr);
    assert(!state.Get().camera_valid);
}
