#pragma once
union SDL_Event;
namespace camera_tools {
// As chamadas de contexto vêm da thread do jogo; desenho e eventos usam app::UiLock.
void SetContext(bool mission, bool paused);
void OpenCheats();
void Draw();
bool RouteEvent(const SDL_Event& event);
bool Visible();
bool Modal();
}
