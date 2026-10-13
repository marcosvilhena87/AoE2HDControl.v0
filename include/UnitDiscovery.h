#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <vector>
namespace aoe2hd {
enum class UnitCandidateReason {
    Accepted, Empty, InvalidBounds, Oversized, Misaligned, UnreadableEntries, NoValidStride
};
const char* UnitCandidateReasonName(UnitCandidateReason reason);
struct UnitVectorCandidate {
    uint32_t offset=0, begin=0, end=0, capacityEnd=0;
    uint32_t count=0, stride=0, readableEntries=0;
    // "World" or "LocalPlayer"; not a claim that this is a unit collection.
    const char* source="LocalPlayer";
    UnitCandidateReason reason=UnitCandidateReason::Accepted;
};
struct UnitDiscoveryReport {
    std::vector<UnitVectorCandidate> candidates;
    uint32_t scannedSlots=0, readFailures=0, nonVectorTriples=0, emptyVectors=0;
    uint32_t invalidBounds=0, oversized=0, misaligned=0, unreadable=0;
};
UnitDiscoveryReport DiagnoseUnitVectors(const MemoryReader& memory,
    const WorldResult& world, uint32_t scanBytes=0x400, uint32_t maxEntries=512);
// Compatibility helper: returns only accepted nonempty candidates.
std::vector<UnitVectorCandidate> DiscoverUnitVectors(const MemoryReader& memory,
    const WorldResult& world, uint32_t scanBytes=0x400, uint32_t maxEntries=512);
}
