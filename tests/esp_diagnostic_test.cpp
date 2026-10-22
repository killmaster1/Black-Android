#include <cassert>
#include "game/esp_diagnostic.h"

int main() {
    game::esp_diagnostic::Diagnostic state;
    state.Tick(1, 10);
    state.Observe(100);
    assert(state.Get().candidates == 0);
    state.Enable(true);
    state.Observe(100); state.Observe(100); state.Observe(200);
    assert(state.Get().candidates == 2 && state.Get().observations == 3);
    state.SetPaused(true);
    state.Tick(1, 1000);
    assert(state.Get().candidates == 2 && state.Get().registered == 2);
    state.Tick(1, 2000);
    assert(state.Get().candidates == 2);
    state.SetPaused(false);
    state.Tick(1, 2120);
    assert(state.Get().candidates == 2);
    state.Tick(1, 2121);
    assert(state.Get().candidates == 0 && state.Get().registered == 2);
    state.Tick(1, 10);
    state.Observe(100); state.Observe(200);
    state.Tick(1, 130);
    assert(state.Get().candidates == 2);
    state.Tick(1, 131);
    assert(state.Get().candidates == 0);
    state.Observe(200);
    assert(state.Get().candidates == 1);
    state.Tick(2, 132);
    assert(state.Get().candidates == 0 && state.Get().observations == 0);
    for (unsigned i=1; i<=200; ++i) state.Observe(i);
    assert(state.Get().candidates == 128);
    state.Tick(2, 1);
    assert(state.Get().candidates == 0);
    state.Reset(); state.Observe(123);
    assert(state.Get().candidates == 0);
    state.Tick(3, 2); state.Observe(123); state.Enable(false);
    assert(!state.Get().enabled && state.Get().candidates == 0);
}
