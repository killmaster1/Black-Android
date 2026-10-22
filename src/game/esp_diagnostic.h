#pragma once
#include <array>
#include <cstdint>
#include <mutex>
#include "game/esp_projection.h"

namespace game::esp_diagnostic {
// Janela de quadros do jogo, congelada durante a pausa. Não representa o total da missão.
struct Marker { Point position{}; uint32_t id = 0, age = 0; float health = 0; };
enum class ListStatus { Waiting, Manager, Header, Shape, Table, Entity, Ready };
inline const char* ListStatusText(ListStatus status) {
    switch (status) {
    case ListStatus::Manager: return "gerenciador indisponivel";
    case ListStatus::Header: return "cabecalho ilegivel";
    case ListStatus::Shape: return "limites invalidos";
    case ListStatus::Table: return "vetor ilegivel";
    case ListStatus::Entity: return "personagem ilegivel";
    case ListStatus::Ready: return "OK";
    default: return "aguardando";
    }
}
struct Live {
    bool valid = false;
    uint32_t count = 0, capacity = 0, used = 0;
    ListStatus status = ListStatus::Waiting;
    std::array<Marker, 128> points{};
};
struct Snapshot {
    bool enabled = false, points_enabled = true, camera_valid = false;
    uint32_t candidates = 0, observations = 0, registered = 0, point_count = 0;
    Matrix camera{};
    Live live{};
    std::array<Marker, 128> points{};
};
class Diagnostic {
    struct Entry { uint32_t address = 0, frame = 0; Point position{}; bool positioned = false; };
    std::mutex lock;
    std::array<Entry, 128> entries{};
    bool enabled = false, paused = false, points_enabled = true, camera_valid = false;
    Matrix camera{};
    uint32_t camera_frame = 0, live_frame = 0;
    Live live{};
    uint32_t world = 0, frame = 0, raw_frame = 0, observations = 0;
    void Clear() { entries = {}; observations = 0; camera_valid = false; live = {}; }
public:
    void Enable(bool value) { std::lock_guard guard(lock); enabled = value; Clear(); }
    void Reset() { std::lock_guard guard(lock); Clear(); world = 0; }
    void SetPaused(bool value) { std::lock_guard guard(lock); paused = value; }
    void Tick(uint32_t current_world, uint32_t current_frame) {
        std::lock_guard guard(lock);
        if (world != current_world || current_frame < raw_frame) {
            Clear(); frame = 0;
        } else if (!paused) {
            frame += current_frame - raw_frame;
        }
        world = current_world; raw_frame = current_frame;
    }
    void PublishLive(const Live& value) {
        std::lock_guard guard(lock);
        if (!enabled || !world) return;
        live = value; live_frame = frame;
    }
    bool Enabled() { std::lock_guard guard(lock); return enabled; }
    void ShowPoints(bool value) { std::lock_guard guard(lock); points_enabled = value; }
    void Camera(const Matrix& value) {
        std::lock_guard guard(lock);
        if (!enabled || !world) return;
        camera_valid = true;
        for (int i = 0; i < 16; ++i)
            if (i % 4 != 3 && !std::isfinite(value[i])) camera_valid = false;
        camera = value; camera_frame = frame;
    }
    void Observe(uint32_t address, const Point* position = nullptr) {
        std::lock_guard guard(lock);
        if (!enabled || !world || !address) return;
        ++observations;
        auto update = [&](Entry& entry) {
            entry = {address, frame, {}, position && Finite(*position)};
            if (entry.positioned) entry.position = *position;
        };
        Entry* replacement = &entries[0];
        for (auto& entry : entries) {
            if (entry.address == address) { update(entry); return; }
            if (!entry.address || frame - entry.frame > frame - replacement->frame) replacement = &entry;
        }
        update(*replacement);
    }
    Snapshot Get() {
        std::lock_guard guard(lock);
        Snapshot result;
        result.enabled = enabled; result.points_enabled = points_enabled;
        result.observations = observations; result.camera = camera;
        result.live = live;
        if (frame - live_frame > 2) result.live = {};
        result.camera_valid = camera_valid && frame - camera_frame <= 2;
        for (const auto& entry : entries) {
            if (!entry.address) continue;
            ++result.registered;
            if (frame - entry.frame <= 120) {
                ++result.candidates;
                if (entry.positioned)
                    result.points[result.point_count++] = {entry.position, uint32_t(&entry - entries.data())+1, frame-entry.frame};
            }
        }
        return result;
    }
};
inline Diagnostic& State() { static Diagnostic state; return state; }
}
