#pragma once
#include <cstdint>
namespace game::ads_ak47 {
// O registro consulta a arma equipada; a alteração das matrizes ocorre apenas na thread do jogo.
bool RegisterCurrent();
void BeforePlayerUpdate(uint32_t player);
void AfterPlayerUpdate(uint32_t player);
}
