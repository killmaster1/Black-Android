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
#include "game/free_camera.h"
#include "game/esp_diagnostic.h"
#include "core/input.h"
#include "game/layout.h"
#include "xdk/hle.h"

// As matrizes usam memória simulada; a interface é desenhada pelo ImGui real, sem GPU.
namespace kernel { uint8_t* g_base; }
namespace app { bool opened = false; bool MenuOpen() { return opened; } void Save() {} }
namespace host { bool Foreground() { return true; } void WindowPixels(int& w,int& h) { w=1280; h=720; } }
namespace input { bool KeyboardActive() { return false; } }
namespace touch { bool LookActive() { return true; } void CancelGestures() {} void ModButtonAnchor(float& x,float& y,float& size) { x=742; y=50; size=40; } }
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
    const auto original_background=ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    camera_tools::SetContext(true,true); camera_tools::OpenCheats(); Frame();
    const auto restored_background=ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    assert(original_background.x==restored_background.x && original_background.w==restored_background.w);
    assert(camera_tools::Visible() && camera_tools::Modal());
    auto* window=ImGui::FindWindowByName("Cheats###ferramentas_camera");
    assert(window && window->Active && window->Size.x>=600 && window->Size.x<=780);
    assert(window->Pos.x+window->Size.x<1280);  // O menu maior cabe na tela durante a pausa.
    // Arrasta de verdade a barra de título e verifica a posição nos quadros seguintes.
    ImVec2 old_pos = window->Pos;
    io.AddMousePosEvent(old_pos.x+80,old_pos.y+12); Frame();
    io.AddMouseButtonEvent(0,true); Frame();
    io.AddMousePosEvent(old_pos.x+160,old_pos.y+70); Frame();
    io.AddMouseButtonEvent(0,false); Frame();
    assert(window->Pos.x > old_pos.x+50 && window->Pos.y > old_pos.y+30);
    auto dragged = window->Pos; Frame();
    assert(window->Pos.x == dragged.x && window->Pos.y == dragged.y);
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

    // Abre diretamente na missão: o modal não depende de uma página de pausa do jogo.
    camera_tools::SetContext(true,false); camera_tools::OpenInMission(); Frame();
    assert(camera_tools::FreezeRequested() && camera_tools::Modal() && camera_tools::Visible());
    e.type=SDL_EVENT_FINGER_DOWN; e.tfinger.fingerID=77; e.tfinger.x=0.99f; e.tfinger.y=0.99f;
    assert(camera_tools::RouteEvent(e));
    e.type=SDL_EVENT_FINGER_UP; assert(camera_tools::RouteEvent(e));
    camera_tools::Close(); Frame();
    assert(!camera_tools::FreezeRequested() && !camera_tools::Modal());
    camera_tools::SetContext(false,false); camera_tools::OpenInMission();
    assert(!camera_tools::FreezeRequested());

    // O texto de diagnóstico aparece em jogo sem abrir um painel ou capturar gestos.
    game::Opt().esp_diagnostic_hud=true;
    game::esp_diagnostic::State().Enable(true);
    camera_tools::SetContext(true,false); Frame();
    auto* hud = ImGui::FindWindowByName("Diagnostico ESP###esp_diagnostic_hud");
    assert(hud && hud->Active && (hud->Flags & ImGuiWindowFlags_NoInputs) == ImGuiWindowFlags_NoInputs);
    assert(!camera_tools::Visible() && !camera_tools::Modal());
    e.type=SDL_EVENT_FINGER_DOWN; e.tfinger.fingerID=99;
    e.tfinger.x=(hud->Pos.x+10)/1280; e.tfinger.y=(hud->Pos.y+10)/720;
    assert(!camera_tools::RouteEvent(e));
    camera_tools::SetContext(true,true); Frame(); assert(!hud->Active);
    camera_tools::SetContext(false,false); Frame(); assert(!hud->Active);
    game::esp_diagnostic::State().Enable(false);
    camera_tools::SetContext(true,false); Frame(); assert(!hud->Active);

    // Linhas e caixas funcionam com pontos e contadores ocultos, sem capturar controles.
    game::Opt().esp_line=game::Opt().esp_box=true;
    game::Opt().esp_diagnostic_hud=false;
    camera_tools::SetContext(true,false); Frame();
    assert(game::esp_diagnostic::State().Enabled());
    game::esp_diagnostic::State().ShowPoints(false);
    game::esp_diagnostic::State().Tick(1,1);
    game::esp_diagnostic::Matrix projection{};
    projection[0]=0.5f; projection[5]=-0.5f;
    projection[8]=projection[9]=0.5f; projection[10]=1;
    game::esp_diagnostic::State().Camera(projection);
    game::esp_diagnostic::Live live; live.valid=true; live.count=1;
    live.points[0]={{0,0,10},1,0};
    game::esp_diagnostic::State().PublishLive(live);
    Frame();
    assert(ImGui::GetForegroundDrawList()->VtxBuffer.Size>0);
    assert(!hud->Active && !camera_tools::Visible() && !camera_tools::Modal());
    game::Opt().esp_line=game::Opt().esp_box=false;
    game::esp_diagnostic::State().Enable(false);

    std::vector<uint8_t> memory(4*1024*1024); kernel::g_base=memory.data();
    constexpr uint32_t world=0x10000, player=world+layout::kWorldPlayer, manager=0x20000;
    kernel::W32(layout::kWorld,world);
    Put(player+0xD0,{1,0,0}); Put(player+0xE0,{0,1,0}); Put(player+0xF0,{0,0,1}); Put(player+0x100,{10,20,30});
    kernel::W32(0x2D12B8,manager);
    Put(manager+0x770,{10,20,30});
    std::vector<uint8_t> original_player(memory.begin()+player, memory.begin()+player+0x900);
    using namespace game::camera_adjust;
    Set({0.1f,0.2f,0.3f,80}); Enable(true);
    BeforeRenderCamera(manager); ApplyRenderCamera(manager);
    auto eye=Read(manager+0x770);
    assert(Near(eye[0],10.1f) && Near(eye[1],20.2f) && Near(eye[2],30.3f));
    ApplyRenderCamera(manager); assert(Read(manager+0x770)==eye);
    Set({0.5f,0,0,80}); ApplyRenderCamera(manager); assert(Near(Read(manager+0x770)[0],10.5f));
    BeforeRenderCamera(manager); assert(Read(manager+0x770)==(Point{10,20,30}));
    ApplyRenderCamera(manager); Enable(false); BeforeRenderCamera(manager);
    assert(Read(manager+0x770)==(Point{10,20,30}));
    assert(std::memcmp(kernel::HostPtr(player),original_player.data(),original_player.size())==0);
    // A câmera livre também só fornece uma posição ao renderizador, sem mover o personagem.
    game::Opt().free_camera=true;
    input::Pad pad{.connected=true};
    game::free_camera::FilterPad(pad);
    game::free_camera::BeforePlayerUpdate(player);
    game::free_camera::View(player);
    Put(manager+0x770,{100,200,300});
    BeforeRenderCamera(manager); ApplyRenderCamera(manager);
    assert(Read(manager+0x770)==(Point{10,20,30}));
    assert(std::memcmp(kernel::HostPtr(player),original_player.data(),original_player.size())==0);
    game::free_camera::Toggle(); BeforeRenderCamera(manager);
    assert(Read(manager+0x770)==(Point{100,200,300}));
    assert(Fov(90)==90); Enable(true); assert(Fov(90)==80);
    StorePreferences(); Set({}); LoadPreferences(); assert(Near(Get().side,0.5f) && Get().fov==80);
    Set({std::numeric_limits<float>::quiet_NaN(),99,-99,999});
    assert(Get().side==0 && Get().height==2 && Get().distance==-2 && Get().fov==140);
    camera_tools::SetContext(false,false); Frame(); assert(!camera_tools::Visible());
    ImGui::DestroyContext();
    std::puts("Painel: passaram desenho ImGui, contexto, multitoque, isolamento dos controles, camera e persistencia.");
}
