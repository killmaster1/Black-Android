#include "model_override.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <mutex>
#include <set>
#include "core/log.h"

namespace model_override {
namespace {
std::mutex g_lock;
std::filesystem::path g_root="/sdcard/Black";
std::set<std::string> g_notices;
void Notice(const std::string& key,const std::string& logical,const std::string& external,uint64_t size,bool active) {
    if (g_notices.size()<64 && g_notices.insert(key).second)
        LOGR("MODELO override_%s recurso=%s externo=%s tamanho=%llu",active?"ativo":"recusado",logical.c_str(),external.c_str(),static_cast<unsigned long long>(size));
}
}
void SetRoot(const std::string& root) {
    std::lock_guard<std::mutex> guard(g_lock);g_root=root;g_notices.clear();
}
bool Wanted(const std::vector<std::string>& parts) {
    if (parts.size()<2) return false;
    for (const auto& p:parts)
        if (p.empty() || p=="." || p==".." || p.find_first_of("/\\:")!=std::string::npos) return false;
    auto lower=[](std::string s) {
        std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return char(std::tolower(c));});return s;
    };
    return lower(parts[parts.size()-2])=="fpguns" && lower(parts.back())=="bg1_pst.db";
}
std::string Find(const std::vector<std::string>& parts,uint64_t original_size) {
    if (!original_size || !Wanted(parts)) return {};
    std::vector<std::string> normalized;
    for (auto part:parts) {
        std::transform(part.begin(),part.end(),part.begin(),[](unsigned char c){return char(std::tolower(c));});
        normalized.push_back(std::move(part));
    }
    std::lock_guard<std::mutex> guard(g_lock);
    std::filesystem::path relative;
    for (const auto& part:normalized) relative/=part;
    const auto specific=g_root/"modelos"/relative;
    const auto global=g_root/"bg1_pst.db";
    for (const auto& candidate:{specific,global}) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(candidate,error) || error) continue;
        uint64_t size=std::filesystem::file_size(candidate,error);
        if (error) continue;
        if (size!=original_size) {
            Notice("recusado:"+relative.string()+candidate.string()+std::to_string(size),relative.string(),candidate.string(),size,false);
            continue;
        }
        Notice("ativo:"+relative.string()+candidate.string(),relative.string(),candidate.string(),size,true);
        return candidate.string();
    }
    return {};
}
}
