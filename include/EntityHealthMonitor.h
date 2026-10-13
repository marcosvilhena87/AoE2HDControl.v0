#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace aoe2hd {
constexpr uint32_t EntityHpFieldOffset=0x34;
struct HealthSample {
 uint32_t base=0,address=0,raw=0;
 float hp=0.0f;
 bool readable=false,plausible=false;
 std::string kind;
};
class EntityHealthMonitor {
public:
 std::vector<HealthSample> observe(const MemoryReader& memory,
    const std::vector<uint32_t>& bases,bool phaseChanged=false);
 void reset(){previous_.clear();}
private:
 std::map<uint32_t,HealthSample> previous_;
};
}
