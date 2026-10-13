#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <string>
#include <vector>
namespace aoe2hd {
struct ConstructionPointerHit {
 std::string root;
 uint32_t rootOffset=0, intermediate=0, intermediateOffset=0, target=0, depth=0;
};
std::vector<ConstructionPointerHit> FindConstructionPointerReferences(
 const MemoryReader& memory,const WorldResult& world,uint32_t target,
 uint32_t scanBytes=0x400,uint32_t maxIntermediates=64);
}
