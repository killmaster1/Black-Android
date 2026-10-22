#pragma once
#include <cstdint>
#include <mutex>
namespace game::npc_spawn {
enum class Status { Idle, Armed, Running, Created, NoRoom, Failed };
struct Snapshot { Status status = Status::Idle; uint32_t attempts = 0, created = 0; uint64_t epoch = 0; };
inline const char* Text(Status value) {
    switch (value) {
    case Status::Armed: return "Aguardando uma criacao natural";
    case Status::Running: return "Criando NPC extra";
    case Status::Created: return "NPC extra criado";
    case Status::NoRoom: return "Sem vaga na reserva de NPCs";
    case Status::Failed: return "Criacao extra nao confirmada";
    default: return "Desligado";
    }
}
class Control {
    std::mutex mutex;
    uint32_t world = 0;
    Snapshot snapshot;
public:
    void World(uint32_t value) {
        std::lock_guard lock(mutex);
        if (world==value) return;
        world=value; uint64_t epoch=snapshot.epoch+1; snapshot={}; snapshot.epoch=epoch;
    }
    bool Arm() {
        std::lock_guard lock(mutex);
        if (!world || snapshot.status==Status::Running) return false;
        snapshot.status=Status::Armed; ++snapshot.epoch; return true;
    }
    void Cancel() {
        std::lock_guard lock(mutex);
        if (snapshot.status==Status::Running) return;
        snapshot.status=Status::Idle; ++snapshot.epoch;
    }
    bool Take(uint32_t value,uint64_t epoch) {
        std::lock_guard lock(mutex);
        if (value!=world || snapshot.epoch!=epoch || snapshot.status!=Status::Armed) return false;
        snapshot.status=Status::Running; ++snapshot.attempts; return true;
    }
    void Finish(uint32_t value,uint64_t epoch,Status status) {
        std::lock_guard lock(mutex);
        if (world!=value || snapshot.epoch!=epoch || snapshot.status!=Status::Running) return;
        snapshot.status=status;
        if (status==Status::Created) ++snapshot.created;
    }
    Snapshot Get() { std::lock_guard lock(mutex); return snapshot; }
};
inline Control& State() { static Control control; return control; }
}
