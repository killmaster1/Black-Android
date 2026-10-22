#include "game/esp_diagnostic.h"
#include "game/layout.h"
#include "xdk/hle.h"

// 0x1BD3BD só é alcançado quando a consulta original encontra IA em entidade+0x39C.
// EDX é a entidade. Não percorremos ponteiros guardados nem alteramos dados do jogo.
void EspObserveCandidate(X86Context& ctx, uint8_t*) {
    game::esp_diagnostic::State().Observe(ctx.edx);
}
