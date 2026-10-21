#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <limits>
#include <mutex>
#include <vector>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "common/camera_tools.h"
#include "common/options.h"
#include "common/panel_input.h"
#include "core/log.h"
#include "game/camera_adjust.h"
#include "game/layout.h"
#include "xdk/hle.h"

// As matrizes usam memória simulada; a interface é desenhada pelo ImGui real, sem GPU.
namespace kernel { uint8_t* g_base; }
namespace app { bool opened = false; bool MenuOpen() { return opened; } void Save() {} }
namespace host { bool Foreground() { return true; } void WindowPixels(int& w,int& h) { w=1280; h=720; } }
namespace input { bool KeyboardActive() { return false; } }
namespace touch { bool LookActive() { return true; } }
namespace ui { void ReleasePointer() {} }
namespace logx { void Write(Level, const char*, ...) {} }
namespace game { Options& Opt() { static Options o; return o; } }

void Frame() { ImGui::NewFrame(); camera_tools::Draw(); ImGui::Render(); }
using Point = std::array<float,3>;
void Put(uint32_t address, Point p) { std::memcpy(kernel::HostPtr(address),p.data(),sizeof(p)); }
Point Read(uint32_t address) { Point p; std::memcpy(p.data(),kernel::HostPtr(address),sizeof(p)); return p; }
bool Near(float a,float b) { return std::abs(a-b)<0.001f; }

int main() {
    camera_tools::Rect rect{10,20,200,300};
    camera_tools::PanelInput router;
    assert(!rect.Contains(640,360) && rect.Contains(15,30));
    assert(router.FingerDown(1,true));
    assert(!router.FingerDown(2,false));
    assert(router.FingerMove(1) && !router.FingerMove(2));
    assert(router.FingerUp(1) && !router.FingerMove(1));
    assert(router.MouseDown(1,true) && router.MouseMove(false) && router.MouseUp(1));
    assert(!router.MouseMove(false));
    assert(!router.MouseDown(1,false) && !router.MouseUp(1));  // Soltar sobre o painel não prende o tiro iniciado fora.

    ImGui::CreateContext();
    auto& io=ImGui::GetIO(); io.IniFilename=nullptr; io.DisplaySize={1280,720}; io.DeltaTime=1.0f/60;
    unsigned char* pixels; int width,height; io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    camera_tools::SetContext(false,false); Frame(); assert(!camera_tools::Visible());
    camera_tools::SetContext(true,true); camera_tools::OpenCheats(); Frame();
    assert(camera_tools::Visible() && camera_tools::Modal());
    auto* window=ImGui::FindWindowByName("Cheats###ferramentas_camera");
    assert(window && window->Active && window->Size.x<=300);
    assert(window->Pos.x+window->Size.x<640);  // O centro da tela fica livre.
    SDL_Event e{}; e.type=SDL_EVENT_FINGER_DOWN; e.tfinger.fingerID=10;
    e.tfinger.x=(window->Pos.x+20)/1280; e.tfinger.y=(window->Pos.y+40)/720;
    assert(camera_tools::RouteEvent(e));
    e.type=SDL_EVENT_FINGER_MOTION; e.tfinger.x=0.9f; assert(camera_tools::RouteEvent(e));
    game::camera_adjust::Enable(true);
    camera_tools::SetContext(true,false); Frame();
    assert(camera_tools::Visible() && !camera_tools::Modal());
    assert(ImGui::GetDrawData()->CmdListsCount>0);
    game::camera_adjust::Enable(false); Frame(); assert(!camera_tools::Visible());
    e.type=SDL_EVENT_FINGER_UP; assert(camera_tools::RouteEvent(e));  // Libera o gesto mesmo depois de ocultar o painel.
    e.type=SDL_EVENT_FINGER_MOTION; assert(!camera_tools::RouteEvent(e));

    std::vector<uint8_t> memory(4*1024*1024); kernel::g_base=memory.data();
    constexpr uint32_t world=0x10000, player=world+layout::kWorldPlayer;
    kernel::W32(layout::kWorld,world);
    Put(player+0xD0,{1,0,0}); Put(player+0xE0,{0,1,0}); Put(player+0xF0,{0,0,1}); Put(player+0x100,{10,20,30});
    using namespace game::camera_adjust;
    Set({0.1f,0.2f,0.3f,80}); Enable(true);
    BeforePlayerUpdate(player); View(player);
    auto eye=Read(player+0x100);
    assert(Near(eye[0],10.1f) && Near(eye[1],20.2f) && Near(eye[2],30.3f));
    View(player); assert(Read(player+0x100)==eye);  // Não acumula o deslocamento entre hooks do mesmo quadro.
    Set({0.5f,0,0,80}); View(player); assert(Near(Read(player+0x100)[0],10.5f));
    BeforePlayerUpdate(player); assert(Read(player+0x100)==(Point{10,20,30}));
    View(player); Enable(false); View(player); assert(Read(player+0x100)==(Point{10,20,30}));
    assert(Fov(90)==90); Enable(true); assert(Fov(90)==80);
    StorePreferences(); Set({}); LoadPreferences(); assert(Near(Get().side,0.5f) && Get().fov==80);
    Set({std::numeric_limits<float>::quiet_NaN(),99,-99,999});
    assert(Get().side==0 && Get().height==2 && Get().distance==-2 && Get().fov==140);
    camera_tools::SetContext(false,false); Frame(); assert(!camera_tools::Visible());
    ImGui::DestroyContext();
    std::puts("Painel: passaram desenho ImGui, contexto, multitoque, isolamento dos controles, camera e persistencia.");
}
