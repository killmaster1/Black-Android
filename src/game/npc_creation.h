#pragma once
#include <array>
#include <cstdint>
#include <mutex>

namespace game::npc_creation {
struct Sample { uint32_t profile = 0; uint64_t resource = 0; bool resource_readable = false, transform_readable = false; };
enum class Result { Created, Empty, Unreadable };
struct Snapshot {
    bool enabled = true;
    uint32_t attempts = 0, created = 0, empty = 0, unreadable = 0, profiles = 0;
    uint32_t last_profile = 0;
    bool has_last = false, last_resource_readable = false, last_transform_readable = false;
    std::array<uint32_t,44> profile_counts{};
};
class Collector {
    std::mutex mutex;
    Snapshot snapshot;
    uint32_t world = 0;
    uint64_t epoch = 1;
    void Clear() { bool enabled=snapshot.enabled; snapshot={}; snapshot.enabled=enabled; ++epoch; }
public:
    bool Enabled() { std::lock_guard lock(mutex); return snapshot.enabled; }
    void Enable(bool enabled) { std::lock_guard lock(mutex); Clear(); snapshot.enabled=enabled; }
    void Reset() { std::lock_guard lock(mutex); Clear(); }
    void World(uint32_t value) { std::lock_guard lock(mutex); if (world!=value) { Clear(); world=value; } }
    uint64_t Begin(uint32_t value) {
        std::lock_guard lock(mutex);
        if (!snapshot.enabled || !value) return 0;
        if (world!=value) { Clear(); world=value; }
        ++snapshot.attempts;
        return epoch;
    }
    void Finish(uint32_t value, uint64_t token, const Sample& sample, Result result) {
        std::lock_guard lock(mutex);
        if (!token || token!=epoch || value!=world || !snapshot.enabled) return;
        if (result==Result::Empty) { ++snapshot.empty; return; }
        if (result==Result::Unreadable || sample.profile>=snapshot.profile_counts.size()) {
            ++snapshot.unreadable; return;
        }
        ++snapshot.created;
        if (!snapshot.profile_counts[sample.profile]++) ++snapshot.profiles;
        snapshot.has_last=true; snapshot.last_profile=sample.profile;
        snapshot.last_resource_readable=sample.resource_readable;
        snapshot.last_transform_readable=sample.transform_readable;
    }
    Snapshot Get() { std::lock_guard lock(mutex); return snapshot; }
};
inline Collector& State() { static Collector collector; return collector; }
}
