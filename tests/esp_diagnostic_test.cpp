#include <cassert>
#include <limits>
#include <cmath>
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
    state.Tick(3, 2);
    using namespace game::esp_diagnostic;
    // Visão para +Z com origem deslocada: eixo X à direita, Y para cima.
    Matrix camera{};
    camera[0] = 0.5f; camera[5] = -0.5f;
    camera[8] = camera[9] = 0.5f; camera[10] = 1;
    camera[12] = -5; // câmera em X=10.
    float x=0, y=0;
    assert(Project({10,0,10}, camera, 1280,720,x,y));
    assert(std::abs(x-640)<0.01f && std::abs(y-360)<0.01f);
    assert(Project({15,5,10}, camera, 1280,720,x,y));
    assert(std::abs(x-960)<0.01f && std::abs(y-180)<0.01f);
    assert(!Project({10,0,-10}, camera,1280,720,x,y));
    assert(!Project({100,0,10}, camera,1280,720,x,y));
    assert(!Project({10,0,0}, camera,1280,720,x,y));
    assert(!Project({10,0,10}, camera,0,720,x,y));
    Point point{10,0,10};
    state.Observe(123,&point); state.Camera(camera);
    auto snapshot=state.Get();
    assert(snapshot.point_count==1 && snapshot.camera_valid && snapshot.points[0].position==point);
    state.ShowPoints(false); assert(!state.Get().points_enabled);
    state.Tick(3,5); assert(!state.Get().camera_valid);
    state.Tick(3,123); assert(state.Get().point_count==0);
    state.Tick(4,124); assert(!state.Get().camera_valid && state.Get().point_count==0);
    point[0]=std::numeric_limits<float>::quiet_NaN();
    state.Observe(123,&point); assert(state.Get().point_count==0);
    camera[0]=std::numeric_limits<float>::infinity();
    state.Camera(camera); assert(!state.Get().camera_valid);
    assert(!Project({10,0,10},camera,1280,720,x,y));
    state.Enable(false);
    assert(!state.Get().enabled && state.Get().candidates == 0);
}
