#pragma once
#include "EntityTypeDiscovery.h"
#include <cstdint>
#include <vector>
namespace aoe2hd {
struct IdleObservation {
 bool idleButtonSelected=false;
 EntityTypeSample fields;
};
struct IdleFieldEvidence {
 uint32_t offset=0,workingCount=0,idleCount=0;
 uint32_t workingDistinct=0,idleDistinct=0;
 bool allReadable=false,disjoint=false;
};
std::vector<IdleFieldEvidence> CompareIdleObservations(const std::vector<IdleObservation>& observations);
}