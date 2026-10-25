#include "model_trace.h"
#include "core/log.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
static std::vector<std::string> lines;
namespace logx {
void Write(Level,const char* fmt,...) {
    char b[4096];va_list a;va_start(a,fmt);std::vsnprintf(b,sizeof(b),fmt,a);va_end(a);lines.emplace_back(b);
}
}
size_t Count(const char* word) { size_t n=0;for(auto& s:lines) if(s.find(word)!=std::string::npos) ++n;return n; }
int main(int argc,char** argv) {
    assert(argc==2);
    const std::string path="D:\\levels\\level_00\\fpguns\\bg1_pst.db";
    const char* known="123456789";
    model_trace::Enable(false,"teste");assert(!model_trace::Wanted(path));
    model_trace::Read(path,9,0,known,9);assert(Count("recurso_completo")==0);
    model_trace::Enable(true,"teste.iso");assert(model_trace::Wanted(path));
    assert(!model_trace::Wanted("D:/levels/level_00/fpguns/bg1_ps5.db"));
    model_trace::Read(path,9,4,known+4,5);assert(Count("recurso_completo")==0);
    model_trace::Read(path,9,4,known+4,5);assert(Count("recurso_completo")==0);
    model_trace::Read(path,9,0,known,4);assert(Count("crc32=cbf43926")==1);
    model_trace::Read(path,9,0,known,9);assert(Count("recurso_completo")==1);
    model_trace::Enable(true,"outro.iso");lines.clear();
    model_trace::Read(path,9,0,known,4);model_trace::Read(path,9,0,"xxxx",4);
    model_trace::Read(path,9,4,known+4,5);assert(Count("bytes_divergentes")==1 && Count("recurso_completo")==0);
    model_trace::Enable(true,"original.iso");lines.clear();
    std::ifstream f(argv[1],std::ios::binary);std::vector<uint8_t> b{std::istreambuf_iterator<char>(f),{}};assert(!b.empty());
    // Páginas fora de ordem simulam leituras fragmentadas/scatter; hash só após cobertura total.
    for(size_t at=4096;at<b.size();at+=4096) model_trace::Read(path,b.size(),at,b.data()+at,std::min(size_t(4096),b.size()-at));
    assert(Count("recurso_completo")==0);
    model_trace::Read(path,b.size(),0,b.data(),std::min(size_t(4096),b.size()));
    assert(Count("recurso_completo")==1 && Count("MODELO malha=")==3);
    std::puts("Leituras filtradas, CRC32 conhecido, fragmentos, cobertura, divergência e descritores: OK");
}
