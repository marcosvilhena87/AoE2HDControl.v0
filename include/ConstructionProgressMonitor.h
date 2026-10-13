#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace aoe2hd {
constexpr uint32_t ConstructionProgressFieldOffset=0xC1C;
struct ConstructionSample {
 uint32_t base=0,address=0,raw=0;
 float value=0.0f;
 bool readable=false,inRange=false;
 std::string kind;
};
class ConstructionProgressMonitor {
public:
 std::vector<ConstructionSample> observe(const MemoryReader& memory,
     const std::vector<uint32_t>& bases,bool phaseChanged=false);
 void reset(){previous_.clear();}
private:
 std::map<uint32_t,ConstructionSample> previous_;
};
}
