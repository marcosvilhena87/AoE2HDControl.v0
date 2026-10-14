#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <vector>
namespace aoe2hd {
struct ReferenceHit {uint32_t address=0,anchor=0;};
std::vector<ReferenceHit> FindExactReferences(const MemoryReader& memory,
 uint32_t begin,uint32_t bytes,uint32_t anchor,uint32_t maxHits=1024);
}