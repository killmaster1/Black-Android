#include "model_obj.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include "kernel/crypto.h"

namespace model_obj {
namespace {
using V=std::array<double,3>; using UV=std::array<double,2>; using Tri=std::array<uint32_t,3>;
struct Corner { int p,uv=-1,n=-1; };
struct Point { V xyz,n;UV uv; };
struct Parsed { std::vector<Point> points;std::vector<Tri> faces;bool missing_uv=false; };
void Need(bool yes,const char* text) { if(!yes) throw std::runtime_error(text); }
uint32_t U32(const std::vector<uint8_t>& b,size_t o) { Need(o+4<=b.size(),"campo fora do DB");uint32_t x;std::memcpy(&x,b.data()+o,4);return x; }
uint16_t U16(const std::vector<uint8_t>& b,size_t o) { Need(o+2<=b.size(),"indice fora do DB");uint16_t x;std::memcpy(&x,b.data()+o,2);return x; }
float F32(const std::vector<uint8_t>& b,size_t o) { uint32_t x=U32(b,o);float f;std::memcpy(&f,&x,4);return f; }
void W32(std::vector<uint8_t>& b,size_t o,uint32_t x) { Need(o+4<=b.size(),"escrita fora do DB");std::memcpy(b.data()+o,&x,4); }
void WF(std::vector<uint8_t>& b,size_t o,double x) { float f=float(x);uint32_t u;std::memcpy(&u,&f,4);W32(b,o,u); }
V Unit(V v) { double n=std::sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);Need(n>=1e-12,"normal nula/face degenerada");for(auto& x:v)x/=n;return v; }
V Normal(V a,V b,V c) { V u,v;for(int e=0;e<3;++e){u[e]=b[e]-a[e];v[e]=c[e]-a[e];}return Unit({u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]}); }
int Index(const std::string& s,size_t count) { size_t end=0;int n=std::stoi(s,&end);Need(end==s.size() && n!=0,"indice OBJ invalido");int64_t x=n>0?int64_t(n)-1:int64_t(count)+n;Need(x>=0 && uint64_t(x)<count,"indice OBJ fora do intervalo");return int(x); }
std::vector<std::array<Corner,3>> Triangulate(const std::vector<Corner>& corners,const std::vector<V>& positions) {
    const size_t count=corners.size();Need(count>=3 && count<=128,"face deve ter de 3 a 128 cantos");
    std::vector<V> v;std::set<V> unique;for(auto c:corners){v.push_back(positions[c.p]);unique.insert(v.back());}Need(unique.size()==count,"posicoes repetidas no poligono");
    V n{};for(int e=0;e<3;++e)for(size_t i=0;i<count;++i)n[e]+=(v[i][(e+1)%3]-v[(i+1)%count][(e+1)%3])*(v[i][(e+2)%3]+v[(i+1)%count][(e+2)%3]);n=Unit(n);
    double scale=1;for(auto xyz:v)for(double x:xyz)scale=std::max(scale,std::abs(x));
    for(auto xyz:v){double d=0;for(int e=0;e<3;++e)d+=(xyz[e]-v[0][e])*n[e];Need(std::abs(d)<=1e-6*scale,"poligono nao plano");}
    int excluded=0;for(int e=1;e<3;++e)if(std::abs(n[e])>std::abs(n[excluded]))excluded=e;
    std::array<int,2> axes{};for(int e=0,k=0;e<3;++e)if(e!=excluded)axes[k++]=e;
    std::vector<UV> xy;for(auto xyz:v)xy.push_back({xyz[axes[0]],xyz[axes[1]]});
    auto cross=[&](size_t a,size_t b,size_t c){return (xy[b][0]-xy[a][0])*(xy[c][1]-xy[a][1])-(xy[b][1]-xy[a][1])*(xy[c][0]-xy[a][0]);};
    double area=0;for(size_t i=0;i<count;++i)area+=xy[i][0]*xy[(i+1)%count][1]-xy[(i+1)%count][0]*xy[i][1];double sign=area>0?1:-1;
    for(size_t i=0;i<count;++i)for(size_t j=i+1;j<count;++j){
        if(j==i+1 || (i==0 && j==count-1))continue;size_t b=(i+1)%count,d=(j+1)%count;
        if(cross(i,b,j)*cross(i,b,d)<=0 && cross(j,d,i)*cross(j,d,b)<=0){
            bool overlap=true;for(int e=0;e<2;++e)overlap&=std::max(std::min(xy[i][e],xy[b][e]),std::min(xy[j][e],xy[d][e]))<=std::min(std::max(xy[i][e],xy[b][e]),std::max(xy[j][e],xy[d][e]));
            Need(!overlap,"poligono com auto-intersecao");
        }
    }
    std::vector<size_t> pending(count);std::iota(pending.begin(),pending.end(),0);std::vector<std::array<Corner,3>> out;
    while(pending.size()>3){bool found=false;
        for(size_t k=0;k<pending.size();++k){size_t a=pending[(k+pending.size()-1)%pending.size()],b=pending[k],c=pending[(k+1)%pending.size()];
            if(cross(a,b,c)*sign<=1e-14)continue;bool contains=false;
            for(auto p:pending)if(p!=a && p!=b && p!=c && cross(a,b,p)*sign>=-1e-14 && cross(b,c,p)*sign>=-1e-14 && cross(c,a,p)*sign>=-1e-14)contains=true;
            if(contains)continue;out.push_back({corners[a],corners[b],corners[c]});pending.erase(pending.begin()+k);found=true;break;
        }Need(found,"triangulacao recusada");
    }
    out.push_back({corners[pending[0]],corners[pending[1]],corners[pending[2]]});return out;
}
Parsed Parse(const std::string& text) {
    Need(text.size()<=4*1024*1024,"OBJ excede 4 MiB");std::vector<V> positions,normals;std::vector<UV> uvs;std::vector<std::array<Corner,3>> triangles;std::set<std::string> materials;
    std::istringstream input(text);std::string line;
    while(std::getline(input,line)){
        line.resize(line.find('#')==std::string::npos?line.size():line.find('#'));std::istringstream row(line);std::string type;if(!(row>>type))continue;
        std::vector<std::string> values;std::string token;while(row>>token){Need(values.size()<512 && token.size()<4096,"linha OBJ excessiva");values.push_back(token);}
        if(type=="v" || type=="vt" || type=="vn"){
            size_t count=type=="vt"?2:3;Need(values.size()==count,"coordenadas OBJ nao suportadas");V v{};
            for(size_t e=0;e<count;++e){size_t end=0;v[e]=std::stod(values[e],&end);Need(end==values[e].size() && std::isfinite(v[e]) && std::abs(v[e])<1e6,"coordenada OBJ invalida");}
            if(type=="v")positions.push_back(v);else if(type=="vn")normals.push_back(v);else uvs.push_back({v[0],v[1]});
            Need(positions.size()<=50000 && normals.size()<=50000 && uvs.size()<=50000,"atributos OBJ excessivos");
        }else if(type=="f"){
            std::vector<Corner> corners;for(auto value:values){
                std::vector<std::string> fields;size_t start=0;
                for(;;){size_t slash=value.find('/',start);fields.push_back(value.substr(start,slash==std::string::npos?slash:slash-start));if(slash==std::string::npos)break;start=slash+1;}
                Need(fields.size()<=3 && !fields[0].empty(),"canto OBJ invalido");Corner c{Index(fields[0],positions.size())};
                if(fields.size()>1 && !fields[1].empty())c.uv=Index(fields[1],uvs.size());if(fields.size()>2 && !fields[2].empty())c.n=Index(fields[2],normals.size());corners.push_back(c);
            }
            auto ts=Triangulate(corners,positions);triangles.insert(triangles.end(),ts.begin(),ts.end());Need(triangles.size()<=1181,"capacidade de indices excedida");
        }else if(type=="usemtl"){std::string material;for(const auto& value:values)material+=" "+value;materials.insert(material);Need(materials.size()<=1,"mais de um material");}
        else Need(type=="o" || type=="g" || type=="s" || type=="mtllib","elemento OBJ nao suportado");
    }
    Need(!triangles.empty(),"OBJ sem triangulos");Parsed out;using Key=std::tuple<int,double,double,double,double,double>;std::map<Key,uint32_t> unique;
    for(auto tri:triangles){V faceNormal=Normal(positions[tri[0].p],positions[tri[1].p],positions[tri[2].p]);Tri face{};
        for(int j=0;j<3;++j){auto c=tri[j];UV uv=c.uv>=0?uvs[c.uv]:UV{};V n=c.n>=0?Unit(normals[c.n]):faceNormal;out.missing_uv|=c.uv<0;
            Key key{c.p,uv[0],uv[1],n[0],n[1],n[2]};auto it=unique.find(key);
            if(it==unique.end()){uint32_t index=uint32_t(out.points.size());unique.emplace(key,index);out.points.push_back({positions[c.p],n,uv});face[j]=index;}else face[j]=it->second;
        }out.faces.push_back(face);
    }Need(out.points.size()<=3687,"capacidade de vertices excedida");return out;
}
uint32_t Packed(V n) { n=Unit(n);auto q=[](double x,int bits){return uint32_t(int32_t(std::nearbyint(x)))&((1u<<bits)-1);};return q(n[0]*1023,11)|(q(n[1]*1023,11)<<11)|(q(n[2]*511,10)<<22); }
}
std::string Sha1(const std::vector<uint8_t>& bytes) {
    alignas(8) uint8_t context[128],digest[20];crypto::ShaInit(context);crypto::ShaUpdate(context,bytes.data(),bytes.size());crypto::ShaFinal(context,digest);
    std::string out;for(auto b:digest){out+="0123456789abcdef"[b>>4];out+="0123456789abcdef"[b&15];}return out;
}
Result Convert(const std::vector<uint8_t>& original,const std::string& obj) {
    Need(original.size()==741896 && Sha1(original)=="5cbdb886f7f18306d815af5364d824b10bfeb1f0","DB original nao homologado para a V3 nativa");
    constexpr size_t first=0x13c30,second=0x43ee0,vertices=5152,box=0x13bd0,indices=0x410b0;
    Need(std::equal(original.begin()+first,original.begin()+first+vertices*36,original.begin()+second),"buffers divergentes");
    Need(U32(original,box+36)==5905 && U32(original,0x13c00+36)==2927,"contadores inesperados");
    Parsed parsed=Parse(obj);V low{1e9,1e9,1e9},high{-1e9,-1e9,-1e9};size_t donor=0;
    for(size_t i=1465;i<vertices;++i){size_t o=first+i*36;
        Need(U32(original,o+12)==255 && U16(original,o+18)==0 && U32(original,o+20)==0,"atributos nao rigidos no grupo da pistola");
        auto bone=U16(original,o+16);Need(bone==108 || bone==111 || bone==114 || bone==117 || bone==120,"indice nao identificado");
        if(bone==108){if(!donor)donor=i;for(int e=0;e<3;++e){double x=F32(original,o+e*4);low[e]=std::min(low[e],x);high[e]=std::max(high[e],x);}}
    }Need(donor==2306,"doador inesperado");
    V modelLow{1e9,1e9,1e9},modelHigh{-1e9,-1e9,-1e9};for(auto p:parsed.points)for(int e=0;e<3;++e){modelLow[e]=std::min(modelLow[e],p.xyz[e]);modelHigh[e]=std::max(modelHigh[e],p.xyz[e]);}
    double scale=1e99;for(int e=0;e<3;++e)if(modelHigh[e]-modelLow[e]>1e-12)scale=std::min(scale,(high[e]-low[e])/(modelHigh[e]-modelLow[e]));scale*=.8;Need(std::isfinite(scale) && scale>0,"escala invalida");
    V translation;for(int e=0;e<3;++e)translation[e]=(low[e]+high[e])/2-scale*(modelLow[e]+modelHigh[e])/2;
    for(auto& p:parsed.points)for(int e=0;e<3;++e){p.xyz[e]=scale*p.xyz[e]+translation[e];Need(p.xyz[e]>=low[e]-1e-7 && p.xyz[e]<=high[e]+1e-7,"posicao fora dos limites");}
    std::vector<uint16_t> strip;for(auto i:parsed.faces[0])strip.push_back(uint16_t(i+1465));
    for(size_t t=1;t<parsed.faces.size();++t){auto f=parsed.faces[t];if((strip.size()+4)%2)std::swap(f[0],f[1]);uint16_t a=uint16_t(f[0]+1465),b=uint16_t(f[1]+1465),c=uint16_t(f[2]+1465);strip.insert(strip.end(),{strip.back(),a,a,b,c});}
    Need(strip.size()<=5905,"indices excedem reserva");Result out;out.db=original;out.vertices=parsed.points.size();out.triangles=parsed.faces.size();out.indices=strip.size();out.missing_uv=parsed.missing_uv;
    std::vector<uint8_t> allowed(original.size());auto permit=[&](size_t o,size_t n){Need(o+n<=allowed.size(),"intervalo invalido");std::fill(allowed.begin()+o,allowed.begin()+o+n,1);};
    for(size_t base:{first,second})for(size_t i=0;i<parsed.points.size();++i){size_t o=base+(i+1465)*36;auto p=parsed.points[i];
        for(int e=0;e<3;++e)WF(out.db,o+e*4,p.xyz[e]);std::copy_n(original.begin()+first+donor*36+12,12,out.db.begin()+o+12);
        W32(out.db,o+24,Packed(p.n));for(int e=0;e<2;++e)WF(out.db,o+28+e*4,p.uv[e]);permit(o,36);
    }
    for(size_t i=0;i<strip.size();++i)std::memcpy(out.db.data()+indices+i*2,&strip[i],2);permit(indices,strip.size()*2);W32(out.db,box+36,uint32_t(strip.size()));permit(box+36,4);
    V newLow{1e9,1e9,1e9},newHigh{-1e9,-1e9,-1e9};for(auto p:parsed.points)for(int e=0;e<3;++e){newLow[e]=std::min(newLow[e],p.xyz[e]);newHigh[e]=std::max(newHigh[e],p.xyz[e]);}
    for(int e=0;e<3;++e){WF(out.db,box+e*4,newHigh[e]);WF(out.db,box+16+e*4,newLow[e]);}permit(box,12);permit(box+16,12);
    for(size_t i=0;i<out.db.size();++i)Need(allowed[i] || out.db[i]==original[i],"escrita nao autorizada");
    Need(std::equal(out.db.begin()+first,out.db.begin()+first+vertices*36,out.db.begin()+second),"copias do buffer inconsistentes");
    for(size_t i=0;i<1465*36;++i)Need(out.db[first+i]==original[first+i] && out.db[second+i]==original[second+i],"maos modificadas");
    std::vector<Tri> decoded;for(size_t i=2;i<strip.size();++i){Tri f{U16(out.db,indices+(i-2)*2),U16(out.db,indices+(i-1)*2),U16(out.db,indices+i*2)};if(i%2)std::swap(f[0],f[1]);if(f[0]!=f[1] && f[0]!=f[2] && f[1]!=f[2])decoded.push_back(f);}
    Need(decoded.size()==parsed.faces.size(),"releitura de triangulos divergente");for(size_t t=0;t<decoded.size();++t)for(int j=0;j<3;++j)Need(decoded[t][j]==parsed.faces[t][j]+1465,"orientacao/indice divergente");return out;
}
}
