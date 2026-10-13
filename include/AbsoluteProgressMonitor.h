#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace aoe2hd {
struct AbsoluteProgressSample {
 uint32_t address=0, value16=0, value32=0;
 bool readable16=false,readable32=false;
 std::string kind;
};
class AbsoluteProgressMonitor {
public:
 std::vector<AbsoluteProgressSample> observe(const MemoryReader& memory,
     const std::vector<uint32_t>& addresses,bool phaseChanged=false);
 void reset(){previous_.clear();}
private:
 std::map<uint32_t,AbsoluteProgressSample> previous_;
};
}
