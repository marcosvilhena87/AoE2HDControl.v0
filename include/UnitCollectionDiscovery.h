#pragma once
#include "PlayerUnitListDiscovery.h"
#include "PlayerUnitEntryProbe.h"
#include "UnitPointerProbe.h"
namespace aoe2hd {
// One consistent snapshot of candidates, ranking and detailed entry evidence.
// These are hypotheses, not proof of a complete authoritative unit collection.
struct UnitCollectionObservation {
 PlayerUnitListCandidate summary;
 std::vector<UnitEntryEvidence> entries;
 UnitPointerEvidence pointers;
};
struct UnitCollectionReport {
 UnitDiscoveryReport discovery;
 std::vector<UnitCollectionObservation> observations;
};
UnitCollectionReport InspectUnitCollections(
 const MemoryReader& memory,const WorldResult& world,
 uint32_t selectedObject=0,uint32_t scanBytes=0x800,
 uint32_t maxEntries=512,uint32_t sampleLimit=64,
 uint32_t moduleBase=0);
}
