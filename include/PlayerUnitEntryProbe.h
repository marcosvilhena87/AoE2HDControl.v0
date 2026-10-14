#pragma once
#include "UnitDiscovery.h"
#include <cstdint>
#include <vector>
namespace aoe2hd {
struct UnitEntryEvidence {
 uint32_t index=0,word=0,value=0,target=0,entityId=0;
 uint32_t owner=0;
 // direct, indirect, numeric_match: only direct/indirect carry verified entity identity.
 const char* kind="";
 bool selectedMatch=false,localOwner=false;
};
std::vector<UnitEntryEvidence> ProbeUnitEntryEvidence(
 const MemoryReader& memory,const WorldResult& world,
 const UnitVectorCandidate& candidate,uint32_t selectedObject=0);
}