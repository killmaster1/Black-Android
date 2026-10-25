#include "model_trace.h"
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>
#include "core/log.h"

namespace model_trace {
namespace {
std::atomic<bool> g_enabled{false};
std::mutex g_lock;
struct Capture { std::vector<uint8_t> bytes, seen; size_t covered=0; bool complete=false, invalid=false; };
std::map<std::string,Capture> g_files;
uint32_t U32(const uint8_t* p) { uint32_t v; std::memcpy(&v,p,4); return v; }
uint32_t Crc(const std::vector<uint8_t>& data) {
    uint32_t crc=~0u;
    for (auto b:data) {
        crc^=b;
        for (unsigned j=0;j<8;++j) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return ~crc;
}
bool Target(std::string& path) {
    for (char& c:path) c=c=='/'?'\\':char(std::toupper(static_cast<unsigned char>(c)));
    constexpr char suffix[]="\\FPGUNS\\BG1_PST.DB";
    constexpr size_t n=sizeof(suffix)-1;
    return path.size()>=n && path.compare(path.size()-n,n,suffix)==0;
}
void Describe(const std::string& path,const std::vector<uint8_t>& b) {
    unsigned meshes=0;
    const uint32_t signature[]={0x41000000,0x41800000,0x41c00000};
    for (size_t r=0;r+128<=b.size();r+=16) {
        if (U32(b.data()+r+24)!=35 || std::memcmp(b.data()+r+44,signature,12)) continue;
        uint64_t g=r+uint64_t(U32(b.data()+r+28)); uint32_t n=U32(b.data()+r+80);
        if (!n || n>32 || g+uint64_t(n)*28>b.size()) continue;
        for (uint32_t j=0;j<n;++j) {
            uint64_t d=g+j*28,v=d+U32(b.data()+d+4),box=d+U32(b.data()+d+12);
            uint32_t stride=U32(b.data()+d+20);
            if ((stride!=24 && stride!=36) || box+48>b.size()) continue;
            uint32_t count=U32(b.data()+box+36); uint64_t ix=box+U32(b.data()+box+40);
            if (ix> b.size() || count>(b.size()-ix)/2 || v>=ix) continue;
            LOGR("MODELO malha=%u caminho=%s raiz=%llx vb=%llx stride=%u indices=%u inicio_indices=%llx",
                 meshes++,path.c_str(),static_cast<unsigned long long>(r),static_cast<unsigned long long>(v),stride,count,static_cast<unsigned long long>(ix));
        }
    }
}
}
void Enable(bool enabled,const std::string& source) {
#ifdef BLACK_TRACE_MODELS
    std::lock_guard<std::mutex> lock(g_lock);
    g_enabled.store(false); g_files.clear();
    LOGR("MODELO fonte_selecionada=%s diagnostico=%s",source.c_str(),enabled?"ativo":"inativo");
    g_enabled.store(enabled);
#else
    (void)enabled; (void)source;
#endif
}
bool Wanted(const std::string& input) {
    if (!g_enabled.load(std::memory_order_relaxed)) return false;
    std::string path=input;return Target(path);
}
void Read(const std::string& input,uint64_t size,uint64_t offset,const void* bytes,size_t count) {
    if (!g_enabled.load(std::memory_order_relaxed) || !count) return;
    std::string path=input;
    if (!Target(path)) return;
    std::lock_guard<std::mutex> lock(g_lock);
    if (!size || size>2*1024*1024 || offset>size || count>size-offset || !bytes) {
        LOGR("MODELO leitura_rejeitada caminho=%s tamanho=%llu offset=%llu bytes=%zu",path.c_str(),static_cast<unsigned long long>(size),static_cast<unsigned long long>(offset),count);
        return;
    }
    auto found=g_files.find(path);
    if (found==g_files.end()) {
        if (g_files.size()>=8) return;
        found=g_files.emplace(path,Capture{}).first;
        found->second.bytes.resize(size);found->second.seen.resize(size);
        LOGR("MODELO primeira_leitura caminho=%s tamanho=%llu offset=%llu bytes=%zu",path.c_str(),static_cast<unsigned long long>(size),static_cast<unsigned long long>(offset),count);
    }
    auto& c=found->second;
    if (c.complete || c.invalid) return;
    if (c.bytes.size()!=size) { c.invalid=true;LOGR("MODELO tamanho_divergente caminho=%s",path.c_str()); return; }
    auto p=static_cast<const uint8_t*>(bytes);
    for (size_t i=0;i<count;++i) {
        size_t at=size_t(offset)+i;
        if (c.seen[at] && c.bytes[at]!=p[i]) {
            c.invalid=true;LOGR("MODELO bytes_divergentes caminho=%s offset=%zu",path.c_str(),at);return;
        }
        if (!c.seen[at]) { c.seen[at]=1;++c.covered; }
        c.bytes[at]=p[i];
    }
    if (c.covered==size) {
        LOGR("MODELO recurso_completo caminho=%s tamanho=%llu crc32=%08x",path.c_str(),static_cast<unsigned long long>(size),Crc(c.bytes));
        Describe(path,c.bytes);c.complete=true;
        c.bytes.clear();c.seen.clear();
    }
}
}
