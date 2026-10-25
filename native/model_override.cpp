#include "model_override.h"
#include "model_obj.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <stdexcept>
#include <mutex>
#include <set>
#include "core/log.h"

namespace model_override {
namespace {
std::mutex g_lock;
std::filesystem::path g_root="/sdcard/Black";
std::set<std::string> g_notices;
std::filesystem::path g_cache;
struct ObjMemo { std::filesystem::file_time_type time;uint64_t size,original_size;std::string result; };
std::map<std::string,ObjMemo> g_objects;
bool ReadBytes(const std::filesystem::path& path,uint64_t limit,std::vector<uint8_t>& bytes) {
    std::error_code error;auto size=std::filesystem::file_size(path,error);
    if(error || size>limit)return false;
    std::ifstream file(path,std::ios::binary);if(!file)return false;
    bytes.resize(size);if(size)file.read(reinterpret_cast<char*>(bytes.data()),size);
    return bool(file) && file.peek()==std::char_traits<char>::eof();
}
void Save(const std::filesystem::path& path,const std::vector<uint8_t>& bytes) {
    auto temporary=path.string()+".tmp";
    {std::ofstream file(temporary,std::ios::binary|std::ios::trunc);if(!file)throw std::runtime_error("cache sem permissao de escrita");
     file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());file.close();if(!file)throw std::runtime_error("falha ao gravar cache");}
    std::filesystem::rename(temporary,path);
}
std::vector<uint8_t> Bytes(const std::string& text) {return {text.begin(),text.end()};}
std::string Obj(const std::filesystem::path& path,uint64_t original_size,const Loader& loader,const std::string& resource) {
    std::error_code error;if(!std::filesystem::is_regular_file(path,error) || error)return {};
    auto size=std::filesystem::file_size(path,error);if(error)return {};
    auto time=std::filesystem::last_write_time(path,error);if(error)return {};
    auto name=path.string();auto memo_key=resource+":"+name;auto old=g_objects.find(memo_key);
    if(old!=g_objects.end() && old->second.time==time && old->second.size==size && old->second.original_size==original_size){
        if(old->second.result.empty())return {};
        if(std::filesystem::is_regular_file(old->second.result,error) && !error && std::filesystem::file_size(old->second.result,error)==original_size && !error)return old->second.result;
    }
    if(g_objects.size()>=16 && old==g_objects.end())return {};
    auto& memo=g_objects[memo_key];memo={time,size,original_size,{}};
    try {
        if(!loader || g_cache.empty())throw std::runtime_error("leitor original/cache indisponivel");
        std::vector<uint8_t> text,original;
        if(!ReadBytes(path,4*1024*1024,text))throw std::runtime_error("OBJ ilegivel ou maior que 4 MiB");
        if(!loader(original) || original.size()!=original_size)throw std::runtime_error("nao foi possivel ler DB original completo");
        auto source_hash=model_obj::Sha1(original),obj_hash=model_obj::Sha1(text);
        if(source_hash!="5cbdb886f7f18306d815af5364d824b10bfeb1f0")throw std::runtime_error("DB original nao homologado para conversao OBJ");
        // Alterar a revisão da chave se o algoritmo de conversão mudar.
        auto key=model_obj::Sha1(Bytes("v3-native-1:"+source_hash+obj_hash));
        std::filesystem::create_directories(g_cache);auto db=g_cache/(key+".db");
        std::vector<uint8_t> stored,stamp;
        if(ReadBytes(db,2*1024*1024,stored) && stored.size()==original_size && ReadBytes(db.string()+".sha1",64,stamp) && std::string(stamp.begin(),stamp.end())==model_obj::Sha1(stored)) {
            LOGR("MODELO obj_cache_validado arquivo=%s cache=%s",name.c_str(),db.string().c_str());memo.result=db.string();return memo.result;
        }
        auto result=model_obj::Convert(original,std::string(text.begin(),text.end()));
        auto hash=model_obj::Sha1(result.db);Save(db,result.db);Save(db.string()+".sha1",Bytes(hash));
        std::string report="{\n  \"versao\": 3,\n  \"original_sha1\": \""+source_hash+"\",\n  \"obj_sha1\": \""+obj_hash+"\",\n  \"db_sha1\": \""+hash+"\",\n  \"vertices\": "+std::to_string(result.vertices)+",\n  \"triangulos\": "+std::to_string(result.triangles)+",\n  \"indices\": "+std::to_string(result.indices)+",\n  \"vinculo\": \"Pistol_Root original\",\n  \"validacao\": \"binaria; efeito visual nao confirmado\"\n}\n";
        auto end=report.rfind('}');
        report.insert(end,"  ,\"grupos_buffers_alterados\": [1, 2],\n  \"grupos_indices_alterados\": [1],\n  \"vertices_reservados_alvo\": 3687,\n  \"indices_reservados\": 5905,\n  \"tamanho_db\": 741896,\n  \"campos_preservados\": [\"maos\", \"grupo 0\", \"offsets externos\"],\n  \"limitacoes\": [\"vinculo rigido ao Pistol_Root\", \"sem materiais novos\", \"sem validacao visual em runtime\"]\n");
        Save(db.string()+".json",Bytes(report));
        LOGR("MODELO obj_convertido arquivo=%s cache=%s vertices=%zu triangulos=%zu indices=%zu uv_ausente=%u",name.c_str(),db.string().c_str(),result.vertices,result.triangles,result.indices,unsigned(result.missing_uv));
        memo.result=db.string();return memo.result;
    }catch(const std::exception& e){LOGR("MODELO obj_recusado arquivo=%s motivo=%s",name.c_str(),e.what());return {};}
}

void Notice(const std::string& key,const std::string& logical,const std::string& external,uint64_t size,bool active) {
    if (g_notices.size()<64 && g_notices.insert(key).second)
        LOGR("MODELO override_%s recurso=%s externo=%s tamanho=%llu",active?"ativo":"recusado",logical.c_str(),external.c_str(),static_cast<unsigned long long>(size));
}
}
void SetRoot(const std::string& root) {
    std::lock_guard<std::mutex> guard(g_lock);g_root=root;g_notices.clear();g_objects.clear();
}
void SetCacheRoot(const std::string& root) {
    std::lock_guard<std::mutex> guard(g_lock);g_cache=root;g_objects.clear();
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
std::string Find(const std::vector<std::string>& parts,uint64_t original_size,const Loader& loader) {
    if (!original_size || original_size>2*1024*1024 || !Wanted(parts)) return {};
    std::vector<std::string> normalized;
    for (auto part:parts) {
        std::transform(part.begin(),part.end(),part.begin(),[](unsigned char c){return char(std::tolower(c));});
        normalized.push_back(std::move(part));
    }
    std::lock_guard<std::mutex> guard(g_lock);
    std::filesystem::path relative;
    for (const auto& part:normalized) relative/=part;
    auto specific_obj=g_root/"modelos"/relative;specific_obj.replace_extension(".obj");
    for (const auto& candidate:{specific_obj,g_root/"bg1_pst.obj"}) {
        auto converted=Obj(candidate,original_size,loader,relative.generic_string());
        if(!converted.empty()) {Notice("ativo:"+relative.string()+converted,relative.string(),converted,original_size,true);return converted;}
    }
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
