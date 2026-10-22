#include "game/camera_adjust.h"
#include "game/esp_diagnostic.h"
#include "game/npc_creation.h"
#include "game/npc_spawn.h"
#include "game/layout.h"
#include "xdk/hle.h"

// EBX é o gerenciador. Os pontos foram conferidos no executável americano fornecido.
void CameraBeforeSetup(X86Context& ctx, uint8_t*) {
    game::npc_spawn::State().World(kernel::R32(layout::kWorld));
    game::npc_creation::State().World(kernel::R32(layout::kWorld));
    game::esp_diagnostic::State().Tick(kernel::R32(layout::kWorld), kernel::R32(0x2F4368));
    game::camera_adjust::BeforeRenderCamera(ctx.ebx);
}
void CameraAfterSetup(X86Context& ctx, uint8_t*) { game::camera_adjust::ApplyRenderCamera(ctx.ebx); }
