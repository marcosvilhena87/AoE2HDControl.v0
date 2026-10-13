#pragma once
#include "UnitDiscovery.h"
#include <cstdint>
#include <string>
#include <vector>
namespace aoe2hd {
struct UnitPointerEvidence {
    std::string source;
    uint32_t offset=0, stride=0, count=0, sampled=0, nonzero=0;
    uint32_t readableTargets=0, plausibleVtables=0, distinctTargets=0;
    uint32_t firstVtable=0, matchingVtables=0;
    uint32_t begin=0;
};
// Limited, read-only structural evidence. Not proof of units or valid C++ objects.
// Excludes duplicated stride interpretations of the same backing array.
std::vector<UnitPointerEvidence> ProbeUnitPointers(
    const MemoryReader& memory, const WorldResult& world,
    const std::vector<UnitVectorCandidate>& candidates,
    uint32_t moduleBase, uint32_t maxSamples=32);
}
