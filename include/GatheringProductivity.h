#pragma once
#include "GatheringProgressDetector.h"
#include <cstdint>
namespace aoe2hd {
struct GatheringProductivity {
 uint64_t observedMs=0,progressMs=0,noProgressMs=0,currentNoProgressMs=0,longestNoProgressMs=0;
 double collected=0,ratePerActiveSecond=0,progressFraction=0;
 bool comparable=false;
};
class GatheringProductivityTracker {
public:
 GatheringProductivity Observe(uint64_t timeMs,const CarryReading& current);
 void Reset();
private:
 CarryReading previous_;
 uint64_t previousTime_=0;
 bool hasPrevious_=false;
 GatheringProductivity totals_;
};
}