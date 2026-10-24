#include "game/camera_adjust.h"
#include "game/esp_diagnostic.h"
#include "game/npc_creation.h"
#include "game/npc_spawn.h"
#include "game/layout.h"
#include "game/mission_lifecycle.h"
#include "game/extreme.h"
#include "common/camera_tools.h"
#include <cmath>
#include "xdk/hle.h"

// EBX é o gerenciador. Os pontos foram conferidos no executável americano fornecido.
void CameraBeforeSetup(X86Context& ctx, uint8_t*) {
    uint32_t world=kernel::R32(layout::kWorld), game_object=kernel::R32(layout::kGame);
    bool mission=world && game_object && kernel::R32(game_object+layout::kGameState)!=game_object+layout::kFrontEnd;
    uint32_t player=world+layout::kWorldPlayer;
    bool alive=mission && kernel::R32(player+0xC4)==2 && !kernel::R32(player+0x384) &&
        std::isfinite(kernel::G<float>(player+0x2F0)) && kernel::G<float>(player+0x2F0)>0;
    static game::MissionLifecycle lifecycle;
    auto events=lifecycle.Update(world,mission,alive,world ? kernel::R32(world+layout::kTimerWholeSteps) : 0);
    if (events.reset) {
        game::npc_spawn::State().Reset();
        game::npc_creation::State().Reset();
        game::esp_diagnostic::State().Reset();
    }
    if (events.started) camera_tools::MissionStarted();
    game::npc_spawn::State().World(world);
    game::npc_creation::State().World(kernel::R32(layout::kWorld));
    game::esp_diagnostic::State().Tick(kernel::R32(layout::kWorld), kernel::R32(0x2F4368));
    game::extreme::Tick();
    game::camera_adjust::BeforeRenderCamera(ctx.ebx);
}
void CameraAfterSetup(X86Context& ctx, uint8_t*) { game::camera_adjust::ApplyRenderCamera(ctx.ebx); }
