#pragma once
#include <cstdint>

namespace input { struct Pad; }

namespace game::free_camera {

// Na thread do jogo: restaura os olhos antes da simulação e modifica a visão depois.
void BeforePlayerUpdate(uint32_t player);
void AfterPlayerUpdate(uint32_t player);
void View(uint32_t player);
// Na leitura dos controles: mantém visão e pausa; direciona movimento e gatilhos à câmera.
void FilterPad(input::Pad& pad);
bool Active();
// Chamado pela linha da pausa, na thread do jogo.
void Toggle();
// O painel gráfico solicita a troca; a thread do jogo aplica a alteração.
void RequestToggle();
void ProcessRequests();

}  // namespace game::free_camera
