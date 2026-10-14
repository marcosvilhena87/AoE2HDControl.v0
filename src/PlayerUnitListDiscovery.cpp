#include "PlayerUnitListDiscovery.h"
#include <algorithm>
#include <set>
namespace aoe2hd {
std::vector<PlayerUnitListCandidate> ProbePlayerUnitLists(const MemoryReader& memory,
 const WorldResult& world,const std::vector<UnitVectorCandidate>& candidates,
 uint32_t selectedObject,uint32_t sampleLimit){
 std::vector<PlayerUnitListCandidate> out;
 sampleLimit=std::min(sampleLimit,128u);
 for(const auto& c:candidates){
  if(c.reason!=UnitCandidateReason::Accepted||!c.stride||
     (c.stride!=4&&c.stride!=8)||!c.count||!c.begin)continue;
  PlayerUnitListCandidate result;result.vector=c;
  std::set<uint32_t> uniqueObjects;
  for(uint32_t i=0;i<std::min(c.count,sampleLimit);++i){
   if(c.begin>UINT32_MAX-i*c.stride)break;
   uint32_t object=0;
   if(!memory.read(c.begin+i*c.stride,&object,4)){++result.unreadable;continue;}
   ++result.sampled;
   if(object<0x10000u||(object&3u)||!uniqueObjects.insert(object).second)continue;
   if(object==selectedObject)++result.selectedMatches;
   auto identity=ReadEntityIdentity(memory,object);
   if(!identity.readable||!identity.candidateId){++result.unreadable;continue;}
   auto ownership=ClassifyEntityOwnership(identity,world);
   if(ownership==EntityOwnership::Local){++result.localMatches;result.sampledIds.push_back(identity.candidateId);}
   else if(ownership==EntityOwnership::OtherPlayer)++result.otherMatches;
  }
  out.push_back(result);
 }
 std::sort(out.begin(),out.end(),[](const PlayerUnitListCandidate& a,const PlayerUnitListCandidate& b){
  if(a.selectedMatches!=b.selectedMatches)return a.selectedMatches>b.selectedMatches;
  if(a.localMatches!=b.localMatches)return a.localMatches>b.localMatches;
  return a.sampled>b.sampled;
 });
 return out;
}
}