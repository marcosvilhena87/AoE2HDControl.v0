#include "ConstructionProgressMonitor.h"
#include <cmath>
#include <cstring>
#include <limits>
namespace aoe2hd {
std::vector<ConstructionSample> ConstructionProgressMonitor::observe(
 const MemoryReader& memory,const std::vector<uint32_t>& bases,bool phaseChanged){
 std::vector<ConstructionSample> output;
 for(uint32_t base:bases){
  ConstructionSample now;now.base=base;
  if(base<=UINT32_MAX-ConstructionProgressFieldOffset){
   now.address=base+ConstructionProgressFieldOffset;
   now.readable=memory.read(now.address,&now.raw,sizeof(now.raw));
   if(now.readable) {
    std::memcpy(&now.value,&now.raw,sizeof(now.value));
    now.inRange=std::isfinite(now.value)&&now.value>=0.0f&&now.value<=100.0f;
   }
  }
  const auto it=previous_.find(base);
  if(it==previous_.end())now.kind="initial";
  else if(it->second.readable!=now.readable)now.kind="status";
  else if(now.readable&&now.raw!=it->second.raw)now.kind="change";
  else if(phaseChanged)now.kind="phase";
  if(!now.kind.empty())output.push_back(now);
  previous_[base]=now;
 }
 return output;
}
}
