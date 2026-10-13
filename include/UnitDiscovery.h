#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <vector>
namespace aoe2hd {
struct UnitVectorCandidate {
    uint32_t offset=0, begin=0, end=0, capacityEnd=0;
    uint32_t count=0, stride=0;
    uint32_t readableEntries=0;
};
// Diagnostic discovery only: candidates are not confirmed unit lists.
std::vector<UnitVectorCandidate> DiscoverUnitVectors(
    const MemoryReader& memory, const WorldResult& world,
    uint32_t scanBytes=0x400, uint32_t maxEntries=512);
}
