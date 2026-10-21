#include "game/camera_adjust.h"
#include "xdk/hle.h"

// EBX é o gerenciador. Os pontos foram conferidos no executável americano fornecido.
void CameraBeforeSetup(X86Context& ctx, uint8_t*) { game::camera_adjust::BeforeRenderCamera(ctx.ebx); }
void CameraAfterSetup(X86Context& ctx, uint8_t*) { game::camera_adjust::ApplyRenderCamera(ctx.ebx); }
