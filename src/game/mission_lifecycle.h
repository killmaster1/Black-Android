#pragma once
#include <cstdint>
namespace game {
struct MissionEvents { bool started=false, reset=false; };
// A pausa comum não encerra a tentativa. Morte, saída e retorno do relógio invalidam os dados antigos.
class MissionLifecycle {
    uint32_t world=0, tick=0;
    bool active=false, alive=false, have_tick=false;
public:
    MissionEvents Update(uint32_t current_world,bool mission,bool current_alive,uint32_t current_tick) {
        MissionEvents result;
        if (!mission || !current_world) {
            result.reset=active; active=false; alive=false; have_tick=false; world=current_world;
            return result;
        }
        if (!active || world!=current_world) {
            active=true;world=current_world;alive=false;have_tick=false;
        }
        if (alive && !current_alive) result.reset=true;
        if (current_alive && !alive) result.started=true;
        if (have_tick && alive && current_alive && current_tick<tick) result.started=result.reset=true;
        alive=current_alive;tick=current_tick;have_tick=true;
        return result;
    }
};
}
