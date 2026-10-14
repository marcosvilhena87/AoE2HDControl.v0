#pragma once
#include "EntityTypeDiscovery.h"
#include <vector>
namespace aoe2hd {
struct ActionTripletField {
 uint32_t offset=0, gathering1=0, stopped=0, gathering2=0;
 bool readable=false, reverses=false, changed=false;
};
std::vector<ActionTripletField> CompareActionTriplet(
 const EntityTypeSample& gathering1,const EntityTypeSample& stopped,
 const EntityTypeSample& gathering2);
}