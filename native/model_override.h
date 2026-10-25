#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Substituição externa restrita ao DB da pistola; não interpreta OBJ em runtime.
namespace model_override {
void SetRoot(const std::string& root);
bool Wanted(const std::vector<std::string>& parts);
std::string Find(const std::vector<std::string>& parts, uint64_t original_size);
}
