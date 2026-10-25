#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// Substituição externa da pistola; OBJ é convertido uma vez e armazenado em cache.
namespace model_override {
using Loader = std::function<bool(std::vector<uint8_t>&)>;
void SetCacheRoot(const std::string& root);
void SetRoot(const std::string& root);
bool Wanted(const std::vector<std::string>& parts);
std::string Find(const std::vector<std::string>& parts, uint64_t original_size, const Loader& loader = {});
}
