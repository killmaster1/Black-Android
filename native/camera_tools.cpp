#include "common/camera_tools.h"
#include "common/panel_input.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
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
#include "game/esp_diagnostic.h"
#include "game/npc_creation.h"
#include "game/npc_spawn.h"
#include "host.h"

namespace camera_tools {
namespace {
std::atomic<bool> g_direct{false}, g_welcome{false};
std::atomic<bool> g_mission{false}, g_paused{true}, g_cheats{false}, g_visible{false};
// Desenho e eventos acessam estes dados sob app::UiLock.
Rect g_bounds, g_launcher_bounds;
ImVec2 g_launcher_position{};
float g_launcher_size=40;
PanelInput g_input;
bool g_minimized = false, g_right = false, g_snap_requested = false;
int g_last_mode = -1;
ImVec2 g_editor_size{}, g_cheats_size{};
float g_mouse_x = 0, g_mouse_y = 0;
const char* g_notice = "";
int g_category = 0;

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
void DrawLauncher() {
    g_launcher_bounds = {};
    if (!g_mission || g_paused || g_welcome || app::MenuOpen() || !host::Foreground()) return;
    float x,y,size; touch::ModButtonAnchor(x,y,size);
    g_launcher_position=ImVec2(x,y); g_launcher_size=size;
    ImGui::SetNextWindowPos(ImVec2(x-size/2,y-size/2),ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    const auto flags=ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("Mods###abrir_mods",nullptr,flags)) {
        if (ImGui::Button("MOD",ImVec2(size,size))) {
            if (FreezeRequested()) Close(); else OpenInMission();
        }
        auto a=ImGui::GetItemRectMin(), b=ImGui::GetItemRectMax();
        g_launcher_bounds={a.x,a.y,b.x-a.x,b.y-a.y};
    }
    ImGui::End(); ImGui::PopStyleVar();
}
void DrawWelcome() {
    auto display=ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(display.x/2,display.y/2),ImGuiCond_Always,ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(std::min(500.0f,display.x-24),0),ImGuiCond_Always);
    const auto flags=ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("Sandro Mods###assinatura_missao",nullptr,flags)) {
        ImGui::SetWindowFontScale(1.35f);
        ImGui::TextWrapped("Mod Menu criado por Sandro Mods");
        ImGui::TextWrapped("Inscreva-se no canal");
        ImGui::Spacing();
        if (ImGui::Button("Ir ao canal",ImVec2(-1,42)))
            SDL_OpenURL("https://youtube.com/@sandromods2022?si=n_ompvindW-w_GC1");
        if (ImGui::Button("Fechar",ImVec2(-1,42))) DismissWelcome();
        auto pos=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
        g_bounds={pos.x,pos.y,size.x,size.y};
    }
    ImGui::End();
}
void DrawDiagnosticHud() {
    const auto diagnostic = game::esp_diagnostic::State().Get();
    if (!diagnostic.enabled || !g_mission || g_paused || FreezeRequested() || !touch::LookActive() ||
        app::MenuOpen() || !host::Foreground()) return;
    const auto display = ImGui::GetIO().DisplaySize;
    uint32_t drawn = 0, offscreen = 0;
    if ((diagnostic.points_enabled || game::Opt().esp_line || game::Opt().esp_box || game::Opt().esp_health || game::Opt().esp_offscreen) && diagnostic.camera_valid) {
        auto* draw = ImGui::GetForegroundDrawList();
        const auto& points = diagnostic.live.valid ? diagnostic.live.points : diagnostic.points;
        const auto count = diagnostic.live.valid ? diagnostic.live.count : diagnostic.point_count;
        for (uint32_t i = 0; i < count; ++i) {
            const auto& point = points[i];
            float x, y;
            bool on_screen=game::esp_diagnostic::Project(point.position, diagnostic.camera, display.x, display.y, x, y);
            // Considera também tronco e cabeça estimados quando os pés saem da imagem.
            if (!on_screen && diagnostic.live.valid) {
                float height=float(std::clamp(game::Opt().esp_box_height,50u,350u))/100;
                for (float share : {0.5f,1.0f}) {
                    auto upper=point.position; upper[1]+=height*share;
                    if (game::esp_diagnostic::Project(upper,diagnostic.camera,display.x,display.y,x,y)) {
                        on_screen=true; break;
                    }
                }
            }
            if (!on_screen) {
                if (diagnostic.live.valid && game::Opt().esp_offscreen &&
                    game::esp_diagnostic::OffscreenDirection(point.position,diagnostic.camera,display.x,display.y,x,y)) {
                    ++offscreen;
                    ImVec2 edge(x,y), origin(display.x*0.5f,display.y*0.78f);
                    const auto red=IM_COL32(255,55,55,230);
                    draw->AddLine(origin,edge,red,1.5f);
                    float dx=x-display.x*0.5f,dy=y-display.y*0.5f,length=std::hypot(dx,dy);
                    if (length>0) {
                        dx/=length;dy/=length;
                        draw->AddTriangleFilled(edge,ImVec2(x-dx*13-dy*5,y-dy*13+dx*5),
                            ImVec2(x-dx*13+dy*5,y-dy*13-dx*5),red);
                    }
                }
                continue;
            }
            ++drawn;
            // Amarelo: leitura recente. Laranja: última posição observada, que pode estar desatualizada.
            auto color = point.age <= 2 ? IM_COL32(255,235,40,255) : IM_COL32(255,145,40,180);
            ImVec2 center(x,y);
            if (diagnostic.live.valid && game::Opt().esp_line)
                draw->AddLine(ImVec2(display.x*0.5f,display.y-12), center, IM_COL32(100,255,100,220), 1.5f);
            if (diagnostic.live.valid && game::Opt().esp_box) {
                game::esp_diagnostic::Box box;
                float height = float(std::clamp(game::Opt().esp_box_height,50u,350u))/100;
                if (game::esp_diagnostic::ProjectBox(point.position,height,diagnostic.camera,display.x,display.y,box)) {
                    draw->AddRect(ImVec2(box.left,box.top),ImVec2(box.right,box.bottom),IM_COL32(0,0,0,220),0,0,3.5f);
                    draw->AddRect(ImVec2(box.left,box.top),ImVec2(box.right,box.bottom),IM_COL32(100,255,100,240),0,0,1.5f);
                }
            }
            if (diagnostic.live.valid && game::Opt().esp_health) {
                char health[40]; std::snprintf(health,sizeof(health),"Vida: %.0f",point.health);
                draw->AddText(ImVec2(x+7,y+8),IM_COL32(0,0,0,255),health);
                draw->AddText(ImVec2(x+6,y+7),IM_COL32(255,255,255,255),health);
            }
            if (!diagnostic.points_enabled) continue;
            draw->AddCircleFilled(center, 5, IM_COL32(0,0,0,220));
            draw->AddCircleFilled(center, 3, color);
            char label[24]; std::snprintf(label, sizeof(label), "%c%u", diagnostic.live.valid ? 'N' : 'C', point.id);
            draw->AddText(ImVec2(x+7,y-8), color, label);
        }
    }
    if (game::Opt().esp_offscreen) {
        char counter[64];
        if (diagnostic.live.valid && diagnostic.camera_valid)
            std::snprintf(counter,sizeof(counter),"Fora da tela: %u",offscreen);
        else std::snprintf(counter,sizeof(counter),"Fora da tela: --");
        auto* draw=ImGui::GetForegroundDrawList();
        auto text=ImGui::CalcTextSize(counter);
        ImVec2 pos(g_launcher_position.x-text.x/2,g_launcher_position.y+g_launcher_size/2+6);
        draw->AddRectFilled(ImVec2(pos.x-4,pos.y-2),ImVec2(pos.x+text.x+4,pos.y+text.y+2),IM_COL32(0,0,0,180),3);
        draw->AddText(pos,IM_COL32(255,50,50,255),counter);
    }
    if (!game::Opt().esp_diagnostic_hud) return;
    // Abaixo da fileira superior de controles, próximo da pausa no layout padrão.
    ImGui::SetNextWindowPos(ImVec2(display.x * 0.527f, display.y * 0.15f), ImGuiCond_Always, ImVec2(0.5f, 0));
    ImGui::SetNextWindowBgAlpha(0.72f);
    const auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;
    if (ImGui::Begin("Diagnostico ESP###esp_diagnostic_hud", nullptr, flags)) {
        ImGui::SetWindowFontScale(1.35f);
        ImGui::Text("ESP teste | Recentes: %u | Registrados: %u", diagnostic.candidates, diagnostic.registered);
        ImGui::Text("Deteccoes da mira: %u", diagnostic.observations);
        if (diagnostic.live.valid) ImGui::Text("Fonte: lista de NPCs | Vivos (teste): %u", diagnostic.live.count);
        else ImGui::TextUnformatted("Fonte: consulta da mira (lista indisponivel)");
        ImGui::Text("Lista v2: %s | Uso: %u/%u", game::esp_diagnostic::ListStatusText(diagnostic.live.status),
                    diagnostic.live.used, diagnostic.live.capacity);
        if (diagnostic.points_enabled)
            ImGui::Text("Pontos na tela: %u | Camera: %s", drawn, diagnostic.camera_valid ? "OK" : "aguardando");
    }
    ImGui::End();
}
void IntSlider(const char* label, uint32_t& value, int minimum, int maximum, const char* format = "%d") {
    int current = int(std::clamp(value,uint32_t(minimum),uint32_t(maximum)));
    ImGui::TextUnformatted(label); ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderInt((std::string("##")+label).c_str(), &current,minimum,maximum,format,ImGuiSliderFlags_NoInput))
        value = uint32_t(current);
}
void CameraSwitches() {
    bool free = game::Opt().free_camera;
    if (ImGui::Checkbox("Camera livre / noclip", &free)) {
        game::free_camera::RequestToggle();
        if (free) game::camera_adjust::Enable(false);
    }
    bool adjust = game::camera_adjust::Enabled();
    if (ImGui::Checkbox("Ajuste de camera", &adjust)) {
        game::camera_adjust::Enable(adjust);
        if (adjust && game::Opt().free_camera) game::free_camera::RequestToggle();
    }
}
void DrawCheats() {
    static constexpr const char* categories[] = {"Jogador", "Armas", "Jogo", "Camera", "ESP", "Diagnostico", "NPCs"};
    float sidebar = std::clamp(ImGui::GetContentRegionAvail().x*0.26f,130.0f,180.0f);
    float height = std::max(90.0f, ImGui::GetContentRegionAvail().y-60);
    ImGui::BeginChild("categorias",ImVec2(sidebar,height),true);
    for (int i=0;i<7;++i)
        if (ImGui::Selectable(categories[i],g_category==i,0,ImVec2(0,36))) g_category=i;
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("opcoes",ImVec2(0,height),true);
    ImGui::TextColored(ImVec4(1,0.40f,0.12f,1),"%s",categories[g_category]);
    ImGui::Separator(); ImGui::Spacing();
    auto& o = game::Opt();
    switch (g_category) {
    case 0:
        ImGui::Checkbox("Invencibilidade",&o.god_mode);
        ImGui::Checkbox("Matar com um tiro",&o.one_hit_kill);
        break;
    case 1:
        ImGui::Checkbox("Municao infinita",&o.infinite_ammo);
        ImGui::Checkbox("Granadas infinitas",&o.infinite_grenades);
        ImGui::Checkbox("Sem recuo",&o.no_recoil);
        IntSlider("Cadencia de tiro",o.rapid_fire,1,4,"%dx");
        ImGui::Separator();
        ImGui::Checkbox("Mira de ferro AK47",&o.ads_ak47);
        if (ImGui::Button("Registrar AK47 equipada",ImVec2(-1,36)))
            g_notice = game::ads_ak47::RegisterCurrent() ? "Arma registrada." : "Equipe a AK47 na missao.";
        ImGui::TextWrapped("Calibre a arma na aba AK47 do painel de ajuste de camera.");
        break;
    case 2:
        ImGui::Checkbox("Modo Extremo",&o.extreme_mode);
        if (o.extreme_mode) {
            IntSlider("Dano de tiros recebido",o.extreme_damage,150,400,"%d%%");
            IntSlider("Vida dos NPCs",o.extreme_npc_health,2,6,"%dx");
            ImGui::TextWrapped("Base Black Ops. Aumenta o desafio alem do original. Pode afetar aliados.");
            if (o.god_mode || o.one_hit_kill)
                ImGui::TextWrapped("Invencibilidade e matar com um tiro reduzem o desafio.");
        }
        IntSlider("Velocidade do jogo",o.game_speed,25,200,"%d%%");
        ImGui::Checkbox("Liberar todas as missoes",&o.unlock_levels);
        ImGui::Checkbox("Liberar Black Ops",&o.unlock_black_ops);
        ImGui::TextWrapped("As liberacoes aparecem na selecao de missoes.");
        break;
    case 3: {
        CameraSwitches();
        IntSlider("Velocidade da camera livre",o.free_camera_speed,25,1000);
        ImGui::TextWrapped("No voo: atirar sobe; granada desce. O ajuste abre um painel lateral ao retomar a missao.");
        bool hidden=game::g_hide_hud.load();
        if (ImGui::Checkbox("Ocultar HUD do jogo",&hidden)) game::g_hide_hud=hidden;
        break;
    }
    case 4: {
        bool enabled=game::esp_diagnostic::State().Enabled();
        if (ImGui::Checkbox("Ativar ESP",&enabled)) {
            game::esp_diagnostic::State().Enable(enabled);
            if (!enabled) o.esp_line=o.esp_box=o.esp_health=o.esp_offscreen=false;
        }
        ImGui::Checkbox("ESP Linha",&o.esp_line);
        ImGui::Checkbox("ESP Caixa",&o.esp_box);
        ImGui::Checkbox("Mostrar vida",&o.esp_health);
        ImGui::Checkbox("Contador e direcoes fora da tela",&o.esp_offscreen);
        float height=float(std::clamp(o.esp_box_height,50u,350u))/100;
        Slider("Altura estimada da caixa",height,0.5f,3.5f);
        o.esp_box_height=uint32_t(std::lround(height*100));
        auto diagnostic=game::esp_diagnostic::State().Get();
        if (ImGui::Checkbox("Pontos dos personagens",&diagnostic.points_enabled))
            game::esp_diagnostic::State().ShowPoints(diagnostic.points_enabled);
        if (ImGui::Button("Desligar ESP",ImVec2(-1,36))) {
            o.esp_line=o.esp_box=o.esp_health=o.esp_offscreen=false;
            game::esp_diagnostic::State().Enable(false);
        }
        ImGui::TextWrapped("Pode incluir aliados. A caixa tem altura estimada; ajuste para a postura do personagem.");
        break;
    }
    case 5: {
        auto diagnostic=game::esp_diagnostic::State().Get();
        if (ImGui::Checkbox("Ativar leitura de NPCs",&diagnostic.enabled)) {
            game::esp_diagnostic::State().Enable(diagnostic.enabled);
            if (!diagnostic.enabled) o.esp_line=o.esp_box=o.esp_health=o.esp_offscreen=false;
        }
        ImGui::Checkbox("Exibir contadores na missao",&game::Opt().esp_diagnostic_hud);
        ImGui::Text("NPCs vivos (teste): %u",diagnostic.live.count);
        ImGui::TextWrapped("Lista v2: %s",game::esp_diagnostic::ListStatusText(diagnostic.live.status));
        ImGui::Text("Uso: %u/%u",diagnostic.live.used,diagnostic.live.capacity);
        ImGui::Text("Deteccoes da mira: %u",diagnostic.observations);
        break;
    }
    }
    if (g_category==6) {
        ImGui::Separator();
        auto spawn=game::npc_spawn::State().Get();
        if (ImGui::Button("Duplicar proximo NPC (teste)",ImVec2(-1,36))) game::npc_spawn::State().Arm();
        if (ImGui::Button("Cancelar duplicacao",ImVec2(-1,36))) game::npc_spawn::State().Cancel();
        ImGui::TextWrapped("%s",game::npc_spawn::Text(spawn.status));
        ImGui::Text("Extras nesta tentativa: %u",spawn.created);
        ImGui::TextWrapped("Retome e avance ate nascer outro NPC. Um extra herda o perfil e o ponto de nascimento. Aliado/inimigo ainda nao pode ser escolhido.");
        ImGui::Separator();
        auto capture=game::npc_creation::State().Get();
        if (ImGui::Checkbox("Observar criacoes naturais",&capture.enabled))
            game::npc_creation::State().Enable(capture.enabled);
        ImGui::Text("Chamadas observadas: %u",capture.attempts);
        ImGui::Text("NPCs criados e reconhecidos: %u",capture.created);
        ImGui::Text("Retornos sem NPC: %u",capture.empty);
        ImGui::Text("Leituras incompletas: %u",capture.unreadable);
        ImGui::Text("Perfis observados: %u",capture.profiles);
        if (capture.has_last) {
            ImGui::Text("Ultimo perfil: %u",capture.last_profile);
            ImGui::Text("Configuracao: %s",capture.last_resource_readable ? "lida" : "indisponivel");
            ImGui::Text("Transformacao: %s",capture.last_transform_readable ? "lida" : "indisponivel");
        }
        if (ImGui::Button("Zerar contadores",ImVec2(-1,36))) game::npc_creation::State().Reset();

    }
    if (*g_notice) ImGui::TextWrapped("%s",g_notice);
    ImGui::EndChild();
    ImGui::Spacing();
    if (ImGui::Button("Salvar perfil",ImVec2(sidebar,38))) Save();
    ImGui::SameLine();
    if (ImGui::Button(g_direct ? "Continuar missao" : "Voltar a pausa",ImVec2(-1,38))) Close();
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
    game::esp_diagnostic::State().SetPaused(paused || g_direct || g_welcome);
    if (!mission) game::esp_diagnostic::State().Reset();
    g_mission = mission; g_paused = paused;
    if (!mission) { g_direct=false; g_welcome=false; }
    if (!mission || !paused) g_cheats = false;
}
void MissionStarted() { g_welcome=true; }
void DismissWelcome() { g_welcome=false; game::esp_diagnostic::State().SetPaused(g_paused || g_direct); }
void OpenCheats() { g_cheats = true; }
void OpenInMission() {
    if (!g_mission || g_paused) return;
    touch::CancelGestures();
    g_direct=true;
    game::esp_diagnostic::State().SetPaused(true);
}
void Close() {
    bool direct=g_direct.exchange(false);
    g_cheats=false;
    if (direct) touch::CancelGestures();
    game::esp_diagnostic::State().SetPaused(g_paused);
}
bool FreezeRequested() { return (g_direct || (g_welcome && !g_paused)) && g_mission; }
bool Visible() { return g_visible.load(); }
bool Modal() { return g_mission && (g_direct || (g_welcome && !g_paused) || (g_cheats && g_paused)); }

void Draw() {
    if (g_welcome && g_mission && !g_paused && !app::MenuOpen() && host::Foreground()) {
        g_launcher_bounds={}; g_bounds={}; g_visible=true;
        ImGui::GetIO().MouseDrawCursor=ImGui::GetIO().MouseSource!=ImGuiMouseSource_TouchScreen;
        game::esp_diagnostic::State().SetPaused(true);
        DrawWelcome(); return;
    }
    if ((game::Opt().esp_line || game::Opt().esp_box || game::Opt().esp_health || game::Opt().esp_offscreen) && !game::esp_diagnostic::State().Enabled())
        game::esp_diagnostic::State().Enable(true);
    DrawLauncher();
    DrawDiagnosticHud();
    g_bounds = {};
    bool cheats = Modal();
    bool editor = g_mission && !g_paused && game::camera_adjust::Enabled() && touch::LookActive();
    bool visible = !app::MenuOpen() && host::Foreground() && (cheats || editor);
    g_visible = visible;
    ImGui::GetIO().MouseDrawCursor = visible && ImGui::GetIO().MouseSource != ImGuiMouseSource_TouchScreen;
    if (!visible) return;
    auto display = ImGui::GetIO().DisplaySize;
    float width = cheats ? std::clamp(display.x*0.62f,520.0f,780.0f) : std::clamp(display.x*0.35f,280.0f,440.0f);
    int mode = cheats ? 0 : g_minimized ? 2 : 1;
    ImVec2 default_size(width, cheats ? std::min(520.0f, display.y*0.82f) : std::min(600.0f, display.y*0.82f));
    ImVec2 wanted = mode == 0 && g_cheats_size.x ? g_cheats_size :
                    mode == 1 && g_editor_size.x ? g_editor_size : default_size;
    if (mode == 2) wanted = ImVec2(g_editor_size.x ? g_editor_size.x : width, 85);
    float x = g_right ? display.x - wanted.x - 8 : 8;
    // A posição inicial fica na lateral; arrastar o título não é desfeito no quadro seguinte.
    ImGui::SetNextWindowPos(ImVec2(x, display.y * 0.10f), g_snap_requested ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    g_snap_requested = false;
    ImGui::SetNextWindowSize(wanted, mode != g_last_mode ? ImGuiCond_Always : ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(std::min(cheats ? 520.0f : 280.0f, display.x-16), mode == 2 ? 65.0f : std::min(cheats ? 300.0f : 200.0f,display.y-16)),
                                       ImVec2(display.x-16, display.y-16));
    ImGui::SetNextWindowBgAlpha(0.90f);
    ImGui::GetIO().ConfigWindowsMoveFromTitleBarOnly = true;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10,8));
    auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    if (mode == 2) flags |= ImGuiWindowFlags_NoResize;
    if (!cheats) flags |= ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(0.025f,0.025f,0.03f,0.97f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg,ImVec4(0.04f,0.04f,0.045f,0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(1,0.25f,0.05f,1));
    ImGui::PushStyleColor(ImGuiCol_TitleBg,ImVec4(0.55f,0.12f,0.02f,1));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive,ImVec4(0.85f,0.22f,0.04f,1));
    ImGui::PushStyleColor(ImGuiCol_CheckMark,ImVec4(1,0.5f,0.12f,1));
    ImGui::PushStyleColor(ImGuiCol_Header,ImVec4(0.5f,0.16f,0.04f,1));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(0.7f,0.22f,0.05f,1));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,ImVec4(0.85f,0.3f,0.05f,1));
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.38f,0.12f,0.04f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(0.7f,0.22f,0.05f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,ImVec4(0.85f,0.3f,0.05f,1));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,2);
    bool content = ImGui::Begin(cheats ? "BLACK - MENU DE CHEATS###ferramentas_camera" : "Ajustes de camera###ferramentas_camera", nullptr, flags);
    ImGui::SetWindowFontScale(cheats ? 1.4f : 1.10f);
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
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(12);
}

bool RouteEvent(const SDL_Event& e) {
    int w, h; host::WindowPixels(w, h);
    switch (e.type) {
        case SDL_EVENT_FINGER_DOWN:
            return g_input.FingerDown(e.tfinger.fingerID, (Modal() || g_bounds.Contains(e.tfinger.x*w, e.tfinger.y*h) || g_launcher_bounds.Contains(e.tfinger.x*w,e.tfinger.y*h)));
        case SDL_EVENT_FINGER_MOTION: return Modal() || g_input.FingerMove(e.tfinger.fingerID);
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_CANCELED: return g_input.FingerUp(e.tfinger.fingerID) || Modal();
        case SDL_EVENT_MOUSE_MOTION:
            if (e.motion.which == SDL_TOUCH_MOUSEID) return false;
            g_mouse_x = e.motion.x; g_mouse_y = e.motion.y;
            return g_input.MouseMove(g_bounds.Contains(g_mouse_x, g_mouse_y));
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (e.button.which == SDL_TOUCH_MOUSEID) return false;
            g_mouse_x = e.button.x; g_mouse_y = e.button.y;
            return g_input.MouseDown(e.button.button, (Modal() || g_bounds.Contains(g_mouse_x,g_mouse_y) || g_launcher_bounds.Contains(g_mouse_x,g_mouse_y)));
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (e.button.which == SDL_TOUCH_MOUSEID) return false;
            return g_input.MouseUp(e.button.button);
        case SDL_EVENT_MOUSE_WHEEL: return (Modal() || g_bounds.Contains(g_mouse_x,g_mouse_y) || g_launcher_bounds.Contains(g_mouse_x,g_mouse_y));
        case SDL_EVENT_KEY_DOWN:
            if (Modal() && (e.key.scancode == SDL_SCANCODE_ESCAPE || e.key.scancode == SDL_SCANCODE_AC_BACK)) {
                if (!e.key.repeat) { if (g_welcome && !g_paused) DismissWelcome(); else Close(); }
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
