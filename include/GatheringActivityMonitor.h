#pragma once
#include "GatheringProgressDetector.h"
#include <cstdint>
#include <deque>
namespace aoe2hd {
enum class GatheringActivity { Unknown, ProgressRecent, NoRecentProgress, CapacityReached, LoadCleared };
const char* GatheringActivityName(GatheringActivity state);
struct GatheringActivityResult {
 GatheringActivity state=GatheringActivity::Unknown;
 float windowGain=0;
 uint64_t windowMs=0;
 bool completeWindow=false;
};
class GatheringActivityMonitor {
public:
 explicit GatheringActivityMonitor(uint64_t windowMs=2000,float epsilon=0.0001f,float capacity=10.f);
 GatheringActivityResult Observe(uint64_t timeMs,const CarryReading& reading);
 void Reset();
private:
 struct Point {uint64_t time;float load;};
 std::deque<Point> points_;
 CarryReading last_;
 uint64_t windowMs_;
 float epsilon_,capacity_;
 bool hasLast_=false;
};
}