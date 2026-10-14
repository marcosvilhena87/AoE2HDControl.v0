#pragma once
#include "WorldReader.h"
#include <cstdint>
namespace aoe2hd {
struct CarryReading {bool valid=false;uint32_t object=0,id=0,owner=0,definition=0,raw=0;float load=0;};
CarryReading ReadSelectedVillagerCarry(const MemoryReader& memory,const WorldResult& world);
enum class GatheringProgress { Baseline, Increased, Unchanged, Decreased, Cleared, IdentityChanged, Unreadable };
GatheringProgress CompareGatheringProgress(const CarryReading& before,const CarryReading& now,float epsilon=0.0001f);
const char* GatheringProgressName(GatheringProgress value);
}