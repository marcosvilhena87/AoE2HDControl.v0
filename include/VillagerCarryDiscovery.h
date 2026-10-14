#pragma once
#include "EntityTypeDiscovery.h"
#include <cstdint>
#include <vector>
namespace aoe2hd {
struct CarryObservation {float hudLoad=0; bool stopped=false; EntityTypeSample sample;};
struct CarryCandidate {uint32_t offset=0, matched=0, total=0, stoppedPairs=0, preservedStopPairs=0;bool perfect=false;};
std::vector<CarryCandidate> RankCarryFields(const std::vector<CarryObservation>& observations);
}