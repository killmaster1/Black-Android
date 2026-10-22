#pragma once
#include <array>
#include <cstdint>
#include <mutex>

namespace game::esp_diagnostic {
// Janela de quadros do jogo, congelada durante a pausa. Não representa o total da missão.
struct Snapshot { bool enabled = false; uint32_t candidates = 0, observations = 0; };
class Diagnostic {
    struct Entry { uint32_t address = 0, frame = 0; };
    std::mutex lock;
    std::array<Entry, 128> entries{};
    bool enabled = false;
    uint32_t world = 0, frame = 0, observations = 0;
    void Clear() { entries = {}; observations = 0; }
public:
    void Enable(bool value) { std::lock_guard guard(lock); enabled = value; Clear(); }
    void Reset() { std::lock_guard guard(lock); Clear(); world = 0; }
    void Tick(uint32_t current_world, uint32_t current_frame) {
        std::lock_guard guard(lock);
        if (world != current_world || current_frame < frame) Clear();
        world = current_world; frame = current_frame;
    }
    void Observe(uint32_t address) {
        std::lock_guard guard(lock);
        if (!enabled || !world || !address) return;
        ++observations;
        Entry* replacement = &entries[0];
        for (auto& entry : entries) {
            if (entry.address == address) { entry.frame = frame; return; }
            if (!entry.address || frame - entry.frame > frame - replacement->frame) replacement = &entry;
        }
        *replacement = {address, frame};
    }
    Snapshot Get() {
        std::lock_guard guard(lock);
        Snapshot result{enabled, 0, observations};
        for (const auto& entry : entries)
            if (entry.address && frame - entry.frame <= 120) ++result.candidates;
        return result;
    }
};
inline Diagnostic& State() { static Diagnostic state; return state; }
}
