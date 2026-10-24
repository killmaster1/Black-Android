#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <mutex>
#include <vector>
namespace game::extreme {
// Mantém a proporção da vida restante ao alterar o multiplicador, sem multiplicações por quadro.
class HealthScale {
    struct Entry { uint64_t identity; uint32_t multiplier; };
    std::mutex mutex;
    std::map<uint32_t,Entry> entries;
public:
    bool Empty() { std::lock_guard lock(mutex); return entries.empty(); }
    void Clear() { std::lock_guard lock(mutex); entries.clear(); }
    void Created(uint32_t actor) { std::lock_guard lock(mutex); entries.erase(actor); }
    float Apply(uint32_t actor,uint64_t identity,float health,uint32_t multiplier) {
        if (!std::isfinite(health) || health<=0) return health;
        multiplier=std::clamp(multiplier,1u,6u);
        std::lock_guard lock(mutex);
        uint32_t previous=1;
        auto it=entries.find(actor);
        if (it!=entries.end() && it->second.identity==identity) previous=it->second.multiplier;
        float result=health*(float(multiplier)/previous);
        if (!std::isfinite(result)) return health;
        if (multiplier==1) entries.erase(actor); else entries[actor]={identity,multiplier};
        return result;
    }
    void Prune(const std::vector<uint32_t>& present) {
        std::lock_guard lock(mutex);
        for (auto it=entries.begin();it!=entries.end();)
            if (std::find(present.begin(),present.end(),it->first)==present.end()) it=entries.erase(it); else ++it;
    }
};
inline HealthScale& Health() { static HealthScale state; return state; }
inline float ExtraDamage(float damage,uint32_t percent,float full_health=750) {
    if (!std::isfinite(damage) || damage<=0) return 0;
    float scaled=damage*float(std::clamp(percent,150u,400u))/100;
    // Não reduz danos originalmente letais. O aumento de um tiro comum fica limitado a 75% da vida cheia.
    return std::max(damage,std::min(scaled,full_health*0.75f))-damage;
}
void Tick();
}
