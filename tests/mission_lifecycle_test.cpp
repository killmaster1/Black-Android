#include <cassert>
#include "game/mission_lifecycle.h"
int main() {
    game::MissionLifecycle lifecycle;
    assert(!lifecycle.Update(0,false,false,0).started);
    assert(!lifecycle.Update(1,true,false,0).started);
    auto event=lifecycle.Update(1,true,true,1);assert(event.started && !event.reset);
    event=lifecycle.Update(1,true,true,1);assert(!event.started && !event.reset);
    event=lifecycle.Update(1,true,true,50);assert(!event.started);
    event=lifecycle.Update(1,true,false,51);assert(event.reset && !event.started);
    event=lifecycle.Update(1,true,false,52);assert(!event.reset);
    event=lifecycle.Update(1,true,true,0);assert(event.started && !event.reset);
    event=lifecycle.Update(1,true,true,20);assert(!event.started);
    event=lifecycle.Update(1,true,true,0);assert(event.started && event.reset);
    event=lifecycle.Update(1,false,true,0);assert(event.reset);
    event=lifecycle.Update(1,true,true,0);assert(event.started);
    event=lifecycle.Update(2,true,true,0);assert(event.started);
}
