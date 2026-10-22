#include "game/esp_diagnostic.h"
#include "game/layout.h"
#include "xdk/hle.h"
#include "kernel/memory.h"
#include <cstring>
#include <limits>

// 0x1BD3BD só é alcançado quando a consulta original encontra IA em entidade+0x39C.
// EDX é a entidade; +0x70 é a transformação e +0xA0 sua translação.
// Esses campos também são inicializados pelo construtor 0x1E03D0.
namespace {
bool Readable(uint32_t address, uint32_t size) {
    if (!address || address > std::numeric_limits<uint32_t>::max() - size) return false;
    for (uint32_t page = address & ~uint32_t(0xFFF);; page += 0x1000) {
        if (!mem::IsCommitted(page)) return false;
        if (page == ((address + size - 1) & ~uint32_t(0xFFF))) return true;
    }
}
}
void EspObserveCandidate(X86Context& ctx, uint8_t*) {
    auto& state = game::esp_diagnostic::State();
    if (!state.Enabled()) return;
    game::esp_diagnostic::Point position;
    if (ctx.edx <= 0xFFFFFF4F && Readable(ctx.edx + 0xA0, sizeof(position))) {
        std::memcpy(position.data(), kernel::HostPtr(ctx.edx + 0xA0), sizeof(position));
        state.Observe(ctx.edx, &position);
    } else state.Observe(ctx.edx);
}

// Depois de 0xD2F40, EDI aponta para a matriz de projeção normalizada já composta.
// Inclui a câmera de renderização atual, inclusive os ajustes de câmera livre.
void EspCaptureProjection(X86Context& ctx, uint8_t*) {
    auto& state = game::esp_diagnostic::State();
    if (!state.Enabled()) return;
    uint32_t display = kernel::R32(layout::kDisplay);
    if (!display || display > 0xFFFF2B6F || ctx.edi != display + 0xD450 || !Readable(ctx.edi, 64)) return;
    game::esp_diagnostic::Matrix matrix;
    std::memcpy(matrix.data(), kernel::HostPtr(ctx.edi), sizeof(matrix));
    state.Camera(matrix);
}
