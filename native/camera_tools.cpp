#include "common/camera_tools.h"
#include "common/panel_input.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <string>
#include <SDL3/SDL.h>
#include <imgui.h>
#include "app/app.h"
#include "common/menu.h"
#include "common/mouse_keyboard.h"
#include "common/options.h"
#include "game/ads_ak47.h"
#include "game/camera_adjust.h"
#include "game/free_camera.h"
#include "host.h"

namespace camera_tools {
namespace {
std::atomic<bool> g_mission{false}, g_paused{true}, g_cheats{false}, g_visible{false};
// Desenho e eventos acessam estes dados sob app::UiLock.
Rect g_bounds;
PanelInput g_input;
bool g_minimized = false, g_right = false, g_snap_requested = false;
int g_last_mode = -1;
ImVec2 g_editor_size{}, g_cheats_size{};
float g_mouse_x = 0, g_mouse_y = 0;
const char* g_notice = "";

void Slider(const char* label, float& value, float minimum, float maximum) {
    ImGui::TextUnformatted(label);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat((std::string("##") + label).c_str(), &value, minimum, maximum, "%.2f", ImGuiSliderFlags_NoInput);
}
void AdsSlider(const char* label, uint32_t& value, bool angle = false) {
    float shown = (float(std::min(value,2000u))-1000) / (angle ? 10 : 100);
    Slider(label, shown, angle ? -15.0f : -0.5f, angle ? 15.0f : 0.5f);
    value = uint32_t(std::lround(shown * (angle ? 10 : 100)) + 1000);
}
void Save() {
    game::camera_adjust::StorePreferences();
    app::Save();
    g_notice = "Perfil salvo.";
}
void DrawCheats() {
    bool free = game::Opt().free_camera;
    if (ImGui::Checkbox("Camera livre", &free)) {
        game::free_camera::RequestToggle();
        if (free) game::camera_adjust::Enable(false);
    }
    bool adjust = game::camera_adjust::Enabled();
    if (ImGui::Checkbox("Ajuste de camera", &adjust)) {
        game::camera_adjust::Enable(adjust);
        if (adjust && game::Opt().free_camera) game::free_camera::RequestToggle();
    }
    ImGui::TextWrapped("Ative o ajuste e retome a missao para usar o painel lateral.");
    ImGui::Separator();
    if (ImGui::Button("Voltar a pausa", ImVec2(-1, 32))) g_cheats = false;
}
void DrawEditor() {
    if (g_minimized) {
        if (ImGui::Button("Ajustar camera", ImVec2(-1, 30))) g_minimized = false;
        return;
    }
    if (ImGui::SmallButton("Recolher")) g_minimized = true;
    ImGui::SameLine();
    if (ImGui::SmallButton(g_right ? "Mover p/ esquerda" : "Mover p/ direita")) { g_right = !g_right; g_snap_requested = true; }
    if (ImGui::BeginTabBar("##ajustes")) {
        if (ImGui::BeginTabItem("Camera")) {
            auto p = game::camera_adjust::Get();
            Slider("Lateral", p.side, -2, 2);
            Slider("Altura", p.height, -2, 2);
            Slider("Distancia", p.distance, -2, 2);
            ImGui::TextUnformatted("Campo de visao");
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderInt("##fov", &p.fov, 45, 140, "%d", ImGuiSliderFlags_NoInput);
            game::camera_adjust::Set(p);
            if (ImGui::Button("Restaurar camera")) game::camera_adjust::Set({});
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("AK47 (ADS)")) {
            ImGui::Checkbox("Ativar ADS", &game::Opt().ads_ak47);
            if (ImGui::Button("Registrar AK47 equipada"))
                g_notice = game::ads_ak47::RegisterCurrent() ? "Arma registrada." : "Equipe a AK47 na missao.";
            auto& o = game::Opt();
            AdsSlider("Lateral da arma", o.ads_ak47_x);
            AdsSlider("Altura da arma", o.ads_ak47_y);
            AdsSlider("Distancia da arma", o.ads_ak47_z);
            AdsSlider("Rotacao horizontal", o.ads_ak47_yaw, true);
            AdsSlider("Inclinacao", o.ads_ak47_pitch, true);
            ImGui::TextWrapped("Use o botao de mirar para conferir o alinhamento.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::Separator();
    if (ImGui::Button("Salvar perfil")) Save();
    ImGui::SameLine();
    if (ImGui::Button("Desativar")) game::camera_adjust::Enable(false);
    if (*g_notice) ImGui::TextWrapped("%s", g_notice);
}
}

void SetContext(bool mission, bool paused) {
    g_mission = mission; g_paused = paused;
    if (!mission || !paused) g_cheats = false;
}
void OpenCheats() { g_cheats = true; }
bool Visible() { return g_visible.load(); }
bool Modal() { return g_cheats && g_paused && g_mission; }

void Draw() {
    g_bounds = {};
    bool cheats = Modal();
    bool editor = g_mission && !g_paused && game::camera_adjust::Enabled() && touch::LookActive();
    bool visible = !app::MenuOpen() && host::Foreground() && (cheats || editor);
    g_visible = visible;
    ImGui::GetIO().MouseDrawCursor = visible && ImGui::GetIO().MouseSource != ImGuiMouseSource_TouchScreen;
    if (!visible) return;
    auto display = ImGui::GetIO().DisplaySize;
    float width = std::clamp(display.x * 0.35f, 280.0f, 440.0f);
    int mode = cheats ? 0 : g_minimized ? 2 : 1;
    ImVec2 default_size(width, cheats ? std::min(300.0f, display.y*0.82f) : std::min(600.0f, display.y*0.82f));
    ImVec2 wanted = mode == 0 && g_cheats_size.x ? g_cheats_size :
                    mode == 1 && g_editor_size.x ? g_editor_size : default_size;
    if (mode == 2) wanted = ImVec2(g_editor_size.x ? g_editor_size.x : width, 85);
    float x = g_right ? display.x - wanted.x - 8 : 8;
    // A posição inicial fica na lateral; arrastar o título não é desfeito no quadro seguinte.
    ImGui::SetNextWindowPos(ImVec2(x, display.y * 0.10f), g_snap_requested ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    g_snap_requested = false;
    ImGui::SetNextWindowSize(wanted, mode != g_last_mode ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(std::min(280.0f, display.x-16), mode == 2 ? 65.0f : 200.0f),
                                       ImVec2(display.x-16, display.y-16));
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10,8));
    auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    if (mode == 2) flags |= ImGuiWindowFlags_NoResize;
    if (!cheats) flags |= ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus;
    bool content = ImGui::Begin(cheats ? "Cheats###ferramentas_camera" : "Ajustes de camera###ferramentas_camera", nullptr, flags);
    ImGui::SetWindowFontScale(1.10f);
    auto pos = ImGui::GetWindowPos(), size = ImGui::GetWindowSize();
    g_right = pos.x + size.x/2 > display.x/2;
    if (mode == 0) g_cheats_size = size;
    if (mode == 1) g_editor_size = size;
    g_last_mode = mode;
    g_bounds = {pos.x, pos.y, size.x, size.y};
    if (content) {
        if (cheats) DrawCheats();
        else DrawEditor();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

bool RouteEvent(const SDL_Event& e) {
    int w, h; host::WindowPixels(w, h);
    switch (e.type) {
        case SDL_EVENT_FINGER_DOWN:
            return g_input.FingerDown(e.tfinger.fingerID, g_bounds.Contains(e.tfinger.x*w, e.tfinger.y*h));
        case SDL_EVENT_FINGER_MOTION: return g_input.FingerMove(e.tfinger.fingerID);
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED: return g_input.FingerUp(e.tfinger.fingerID);
        case SDL_EVENT_MOUSE_MOTION:
            if (e.motion.which == SDL_TOUCH_MOUSEID) return false;
            g_mouse_x = e.motion.x; g_mouse_y = e.motion.y;
            return g_input.MouseMove(g_bounds.Contains(g_mouse_x, g_mouse_y));
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (e.button.which == SDL_TOUCH_MOUSEID) return false;
            g_mouse_x = e.button.x; g_mouse_y = e.button.y;
            return g_input.MouseDown(e.button.button, g_bounds.Contains(g_mouse_x,g_mouse_y));
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.button.which == SDL_TOUCH_MOUSEID) return false;
            return g_input.MouseUp(e.button.button);
        case SDL_EVENT_MOUSE_WHEEL: return g_bounds.Contains(g_mouse_x,g_mouse_y);
        case SDL_EVENT_KEY_DOWN:
            if (Modal() && (e.key.scancode == SDL_SCANCODE_ESCAPE || e.key.scancode == SDL_SCANCODE_AC_BACK)) {
                if (!e.key.repeat) g_cheats = false;
                return true;
            }
            return Modal();
        case SDL_EVENT_KEY_UP: case SDL_EVENT_TEXT_INPUT: return Modal();
        case SDL_EVENT_WILL_ENTER_BACKGROUND:
            g_input.Clear();
            ui::ReleasePointer();
            return false;
        default: return false;
    }
}
}
