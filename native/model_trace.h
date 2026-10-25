#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

// Diagnóstico optativo: só observa leituras do recurso da pistola, sem alterar bytes.
namespace model_trace {
void Enable(bool enabled, const std::string& source);
bool Wanted(const std::string& path);
void Read(const std::string& path, uint64_t size, uint64_t offset, const void* bytes, size_t count);
}
