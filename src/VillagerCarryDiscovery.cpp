#include "VillagerCarryDiscovery.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace aoe2hd {
std::vector<CarryCandidate> RankCarryFields(const std::vector<CarryObservation>& observations){
 std::vector<CarryCandidate> out;
 if(observations.size()<2)return out;
 size_t count=observations.front().sample.fields.size();
 for(const auto& o:observations)count=std::min(count,o.sample.fields.size());
 for(size_t k=0;k<count;++k){
  CarryCandidate c;c.offset=observations.front().sample.fields[k].offset;
  bool valid=true;
  for(const auto& o:observations){
   const auto& f=o.sample.fields[k];
   if(!f.readable||f.offset!=c.offset){valid=false;break;}
   float asFloat=0;std::memcpy(&asFloat,&f.raw,sizeof(asFloat));
   if(std::isfinite(asFloat)&&std::fabs(asFloat-o.hudLoad)<0.051f)++c.matched;
   ++c.total;
  }
  if(!valid)continue;
  for(size_t i=1;i<observations.size();++i){
   const auto& prev=observations[i-1];const auto& cur=observations[i];
   if(cur.stopped&&std::fabs(cur.hudLoad-prev.hudLoad)<0.051f){
    ++c.stoppedPairs;
    if(cur.sample.fields[k].raw==prev.sample.fields[k].raw)++c.preservedStopPairs;
   }
  }
  c.perfect=c.total==c.matched&&c.stoppedPairs==c.preservedStopPairs;
  out.push_back(c);
 }
 std::sort(out.begin(),out.end(),[](const CarryCandidate& a,const CarryCandidate& b){
  if(a.matched!=b.matched)return a.matched>b.matched;
  if(a.preservedStopPairs!=b.preservedStopPairs)return a.preservedStopPairs>b.preservedStopPairs;
  return a.offset<b.offset;
 });
 return out;
}
}