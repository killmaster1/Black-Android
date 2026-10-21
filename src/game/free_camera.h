#pragma once
#include <cstdint>

namespace input { struct Pad; }

namespace game::free_camera {

// game thread: restore the real eye before simulation, override only the view afterwards
void BeforePlayerUpdate(uint32_t player);
void AfterPlayerUpdate(uint32_t player);
void View(uint32_t player);
// input polling thread: keep looking and pause, redirect movement and triggers to the camera
void FilterPad(input::Pad& pad);
bool Active();

}  // namespace game::free_camera
