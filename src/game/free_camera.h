#pragma once
#include <cstdint>
#include <array>

namespace input { struct Pad; }

namespace game::free_camera {

// Na thread do jogo: atualiza apenas a posição independente da câmera.
void BeforePlayerUpdate(uint32_t player);
void AfterPlayerUpdate(uint32_t player);
void View(uint32_t player);
// Na leitura dos controles: mantém visão e pausa; direciona movimento e gatilhos à câmera.
void FilterPad(input::Pad& pad);
bool Active();
// Fornece a posição ao renderizador, sem escrever nos campos do personagem.
bool Position(std::array<float, 3>& position);
// Chamado pela linha da pausa, na thread do jogo.
void Toggle();
// O painel gráfico solicita a troca; a thread do jogo aplica a alteração.
void RequestToggle();
void ProcessRequests();

}  // namespace game::free_camera
