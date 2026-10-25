#include "model_override.h"
#include "core/log.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
static std::vector<std::string> logs;
namespace logx {
void Write(Level,const char* fmt,...) { char b[4096];va_list a;va_start(a,fmt);std::vsnprintf(b,sizeof(b),fmt,a);va_end(a);logs.emplace_back(b); }
}
void Save(const std::filesystem::path& p,const char* s) { std::filesystem::create_directories(p.parent_path());std::ofstream(p)<<s; }
int main(int argc,char** argv) {
    assert(argc==2);std::filesystem::path root=argv[1];
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
    std::puts("Override: prioridade específica/global, fallback, tamanho, filtro, traversal e logs limitados: OK");
}
