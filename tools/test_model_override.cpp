#include "model_override.h"
#include "model_obj.h"
#include "core/log.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
static std::vector<std::string> logs;
namespace logx {
void Write(Level,const char* fmt,...) { char b[4096];va_list a;va_start(a,fmt);std::vsnprintf(b,sizeof(b),fmt,a);va_end(a);logs.emplace_back(b); }
}
void Save(const std::filesystem::path& p,const char* s) { std::filesystem::create_directories(p.parent_path());std::ofstream(p)<<s; }
int main(int argc,char** argv) {
    assert(argc==4);std::filesystem::path root=argv[1];
    const std::vector<std::string> parts={"levels","level_00","fpguns","bg1_pst.db"};
    model_override::SetRoot(root.string());assert(model_override::Find(parts,3).empty());
    Save(root/"bg1_pst.db","abc");
    assert(model_override::Find(parts,9).empty());
    assert(model_override::Find(parts,3)==(root/"bg1_pst.db").string());
    auto count=logs.size();for(int n=0;n<100;++n) model_override::Find(parts,3);assert(logs.size()==count);
    auto specific=root/"modelos/levels/level_00/fpguns/bg1_pst.db";Save(specific,"def");
    assert(model_override::Find(parts,3)==specific.string());
    Save(specific,"tamanho errado");assert(model_override::Find(parts,3)==(root/"bg1_pst.db").string());
    assert(model_override::Find(parts,0).empty());
    assert(model_override::Find({"levels","..","fpguns","bg1_pst.db"},3).empty());
    assert(model_override::Find({"fpguns","bg1_ps5.db"},3).empty());
    assert(model_override::Find({"outra","bg1_pst.db"},3).empty());
    assert(model_override::Find({"LEVELS","LEVEL_00","FPGUNS","BG1_PST.DB"},3)==(root/"bg1_pst.db").string());
    model_override::SetRoot((root/"vazio").string());assert(model_override::Find(parts,3).empty());
    assert(std::filesystem::file_size(root/"bg1_pst.db")==3);
    auto read=[](const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return std::vector<uint8_t>{std::istreambuf_iterator<char>(f),{}};};
    auto save=[](const std::filesystem::path& p,const std::vector<uint8_t>& b){std::filesystem::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(b.data()),b.size());};
    const auto original=read(argv[2]),object=read(argv[3]);assert(!original.empty() && !object.empty());
    const auto expected=model_obj::Convert(original,std::string(object.begin(),object.end())).db;
    model_override::SetRoot(root.string());model_override::SetCacheRoot((root/"cache").string());
    save(root/"bg1_pst.db",original);save(root/"bg1_pst.obj",object);
    unsigned reads=0;auto loader=[&](std::vector<uint8_t>& b){++reads;b=original;return true;};
    std::filesystem::path converted=model_override::Find(parts,original.size(),loader);
    assert(converted!=root/"bg1_pst.db" && read(converted)==expected && reads==1);
    assert(read(root/"bg1_pst.obj")==object && read(root/"bg1_pst.db")==original);
    assert(model_override::Find(parts,original.size(),loader)==converted.string() && reads==1);
    // O mesmo OBJ global não pode autorizar outra fase sem conferir seu original.
    auto wrong_loader=[&](std::vector<uint8_t>& b){b=original;b[0]^=1;return true;};
    assert(model_override::Find({"levels","level_03","fpguns","bg1_pst.db"},original.size(),wrong_loader)==(root/"bg1_pst.db").string());
    model_override::SetRoot(root.string());
    assert(model_override::Find(parts,original.size(),loader)==converted.string());
    bool cache_report=false;for(const auto& l:logs)cache_report|=l.find("obj_cache_validado")!=std::string::npos;assert(cache_report);
    auto corrupt=read(converted);corrupt[0]^=1;save(converted,corrupt);model_override::SetRoot(root.string());
    assert(model_override::Find(parts,original.size(),loader)==converted.string() && read(converted)==expected);
    Save(root/"bg1_pst.obj","v nan 0 0\nf 1 1 1\n");model_override::SetRoot(root.string());
    assert(model_override::Find(parts,original.size(),loader)==(root/"bg1_pst.db").string());
    std::filesystem::remove(root/"bg1_pst.db");
    assert(model_override::Find(parts,original.size(),loader).empty());
    auto phase_obj=root/"modelos/levels/level_00/fpguns/bg1_pst.obj";save(phase_obj,object);model_override::SetRoot(root.string());
    assert(read(model_override::Find(parts,original.size(),loader))==expected);
    assert(read(argv[2])==original && read(argv[3])==object);
    std::puts("OBJ integrado: prioridade, equivalência, memoização, cache persistente, corrupção, fase incompatível e fallback: OK");
    std::puts("Override: prioridade específica/global, fallback, tamanho, filtro, traversal e logs limitados: OK");
}
