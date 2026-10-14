#include "IdleVillagerDiscovery.h"
#include <algorithm>
#include <set>
namespace aoe2hd {
std::vector<IdleFieldEvidence> CompareIdleObservations(const std::vector<IdleObservation>& observations){
 std::vector<IdleFieldEvidence> result;
 if(observations.empty())return result;
 size_t n=observations.front().fields.fields.size();
 for(const auto& s:observations)n=std::min(n,s.fields.fields.size());
 for(size_t k=0;k<n;++k){
  IdleFieldEvidence e;e.offset=observations.front().fields.fields[k].offset;
  std::set<uint32_t> working,idle;
  e.allReadable=true;
  for(const auto& obs:observations){
   const auto& f=obs.fields.fields[k];
   if(!f.readable||f.offset!=e.offset){e.allReadable=false;break;}
   if(obs.idleButtonSelected){++e.idleCount;idle.insert(f.raw);}
   else{++e.workingCount;working.insert(f.raw);}
  }
  if(e.allReadable){
   e.workingDistinct=static_cast<uint32_t>(working.size());
   e.idleDistinct=static_cast<uint32_t>(idle.size());
   e.disjoint=!working.empty()&&!idle.empty();
   for(auto value:working)if(idle.count(value))e.disjoint=false;
  }
  result.push_back(e);
 }
 return result;
}
}