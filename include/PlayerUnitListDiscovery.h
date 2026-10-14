#pragma once
#include "UnitDiscovery.h"
#include "EntityIdentityReader.h"
#include <vector>
namespace aoe2hd {
struct PlayerUnitListCandidate {
 UnitVectorCandidate vector;
 uint32_t sampled=0,localMatches=0,otherMatches=0,unreadable=0,selectedMatches=0;
 std::vector<uint32_t> sampledIds;
};
std::vector<PlayerUnitListCandidate> ProbePlayerUnitLists(const MemoryReader& memory,
 const WorldResult& world,const std::vector<UnitVectorCandidate>& candidates,
 uint32_t selectedObject=0,uint32_t sampleLimit=64);
}