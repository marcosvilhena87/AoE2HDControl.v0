#include "EntityHealthMonitor.h"
#include <cmath>
#include <cstring>
namespace aoe2hd {
std::vector<HealthSample> EntityHealthMonitor::observe(
 const MemoryReader& memory,const std::vector<uint32_t>& bases,bool phaseChanged){
 std::vector<HealthSample> events;
 for(uint32_t base:bases){
  HealthSample now;now.base=base;
  if(base<=UINT32_MAX-EntityHpFieldOffset){
   now.address=base+EntityHpFieldOffset;
   now.readable=memory.read(now.address,&now.raw,sizeof(now.raw));
   if(now.readable){
    std::memcpy(&now.hp,&now.raw,sizeof(now.hp));
    now.plausible=std::isfinite(now.hp)&&now.hp>=0.0f&&now.hp<=100000.0f;
   }
  }
  auto old=previous_.find(base);
  if(old==previous_.end())now.kind="initial";
  else if(old->second.readable!=now.readable)now.kind="status";
  else if(now.readable&&old->second.raw!=now.raw)now.kind="change";
  else if(phaseChanged)now.kind="phase";
  if(!now.kind.empty())events.push_back(now);
  previous_[base]=now;
 }
 return events;
}
}
