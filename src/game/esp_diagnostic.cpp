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
// 0x1CDA30 percorre manager+0x79BC, com contagem em +0x79B4.
// 0x1E6860 compara essa contagem à capacidade em +0x79B8.
// 0x1CDDE0 distingue personagem tipo 1 e consulta seu estado de morte em +0x384.
game::esp_diagnostic::Live ReadLive() {
    game::esp_diagnostic::Live result;
    uint32_t manager = kernel::R32(0x2D1980);
    if (!manager || manager > 0xFFFF863F || !Readable(manager + 0x79B4, 12)) return result;
    uint32_t count = kernel::R32(manager + 0x79B4);
    uint32_t capacity = kernel::R32(manager + 0x79B8);
    uint32_t table = kernel::R32(manager + 0x79BC);
    if (!capacity || count > capacity || capacity > result.points.size()) return result;
    if (count && !Readable(table, count * 4)) return result;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t entity = kernel::R32(table + i*4);
        if (!entity) continue;
        if (entity > 0xFFFFFC5F || !Readable(entity, 0x3A0)) return {};
        if (kernel::R32(entity + 0xC4) != 1 || !kernel::R32(entity + 0x39C) ||
            kernel::R32(entity + 0x384)) continue;
        float health;
        std::memcpy(&health, kernel::HostPtr(entity + 0x2F0), sizeof(health));
        if (!std::isfinite(health) || health <= 0) continue;
        game::esp_diagnostic::Point position;
        std::memcpy(position.data(), kernel::HostPtr(entity + 0xA0), sizeof(position));
        if (!game::esp_diagnostic::Finite(position)) continue;
        result.points[result.count++] = {position, i+1, 0};
    }
    result.valid = true;
    return result;
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
    state.PublishLive(ReadLive());
}
