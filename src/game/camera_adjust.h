#pragma once
#include <cstdint>
namespace game::camera_adjust {
struct Parameters { float side = 0, height = 0, distance = 0; int fov = 70; };
Parameters Get();
void Set(Parameters parameters);
bool Enabled();
void Enable(bool enabled);
void LoadPreferences();
void StorePreferences();
uint32_t Fov(uint32_t original);
// Hooks da câmera de renderização; não modificam a matriz do jogador.
void BeforeRenderCamera(uint32_t manager);
void ApplyRenderCamera(uint32_t manager);
}
