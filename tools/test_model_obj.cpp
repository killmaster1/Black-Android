#include "model_obj.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
std::vector<uint8_t> Read(const char* path) {std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
void Reject(const std::vector<uint8_t>& db,const std::string& text) {try{model_obj::Convert(db,text);}catch(const std::exception&){return;}throw std::runtime_error("Entrada incompatível aceita");}
int main(int argc,char** argv) {
    assert(argc==4);auto original=Read(argv[1]);auto text=Read(argv[2]);std::string obj(text.begin(),text.end());
    auto r=model_obj::Convert(original,obj);assert(r.vertices==24 && r.triangles==12 && r.indices==58 && !r.missing_uv);
    assert(r.db==model_obj::Convert(original,obj).db);assert(original==Read(argv[1]));
    {std::ofstream out(argv[3],std::ios::binary);out.write(reinterpret_cast<const char*>(r.db.data()),r.db.size());}
    auto wrong=original;wrong[0]^=1;Reject(wrong,obj);
    Reject(original,"v nan 0 0\nf 1 1 1\n");Reject(original,"v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 9\n");
    Reject(original,"v 0 0 0\nv 1 0 0\nv 1 1 1\nv 0 1 0\nf 1 2 3 4\n");
    Reject(original,"v 0 0 0\nv 1 1 0\nv 0 1 0\nv 1 0 0\nf 1 2 3 4\n");
    auto concave=model_obj::Convert(original,"v 0 0 0\nv 2 0 0\nv 2 2 0\nv 1 1 0\nv 0 2 0\nf -5 -4 -3 -2 -1\n");
    assert(concave.triangles==3 && concave.missing_uv);
    std::string excess=obj;for(int i=0;i<2000;++i)excess+="f 1 2 3\n";Reject(original,excess);
    Reject(original,std::string(4*1024*1024+1,' '));
    std::cout<<"OBJ nativo: determinismo, limites, triangulação, normais/UV, fonte homologada e recusas: OK\n";
}
