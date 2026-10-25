#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
namespace model_obj {
struct Result {
    std::vector<uint8_t> db;
    size_t vertices=0, triangles=0, indices=0;
    bool missing_uv=false;
};
std::string Sha1(const std::vector<uint8_t>& bytes);
// Porta limitada da V3: original homologado, Pistol_Root, capacidade fixa e ajuste automático.
Result Convert(const std::vector<uint8_t>& original, const std::string& obj);
}
