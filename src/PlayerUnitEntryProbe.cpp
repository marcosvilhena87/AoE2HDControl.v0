#include "PlayerUnitEntryProbe.h"
#include "EntityIdentityReader.h"
#include <set>
namespace aoe2hd {
std::vector<UnitEntryEvidence> ProbeUnitEntryEvidence(
 const MemoryReader& memory,const WorldResult& world,
 const UnitVectorCandidate& c,uint32_t selectedObject){
 std::vector<UnitEntryEvidence> result;
 if(c.reason!=UnitCandidateReason::Accepted||!c.begin||!c.count||
    (c.stride!=4&&c.stride!=8)||c.count>1024)return result;
 std::set<uint32_t> seen;
 const auto* local=GetLocalPlayer(world);
 for(uint32_t i=0;i<c.count;++i){
  if(i>(UINT32_MAX-c.begin)/c.stride)break;
  const uint32_t entry=c.begin+i*c.stride;
  for(uint32_t w=0;w<c.stride/4;++w){
   uint32_t value=0;
   if(!memory.read(entry+w*4,&value,4)||!value)continue;
   if(selectedObject&&value==selectedObject){
    result.push_back({i,w,value,value,0,0,"direct_anchor",true,false});
   }
   if(value<0x10000u||(value&3u))continue;
   // First hop: direct object pointer; second hop: first two dwords of an intermediate node.
   for(uint32_t depth=0;depth<=1;++depth){
    if(depth==1&&value>UINT32_MAX-8u)continue;
    for(uint32_t slot=0;slot<(depth?2u:1u);++slot){
     uint32_t target=value;
     if(depth&&!memory.read(value+slot*4,&target,4))continue;
     if(target<0x10000u||(target&3u))continue;
     auto identity=ReadEntityIdentity(memory,target);
     const bool selected=selectedObject&&target==selectedObject;
     const bool owned=identity.readable&&identity.candidateId&&local&&
        identity.candidateOwner==local->object;
     if(!selected&&!owned)continue;
     const uint32_t pathWord=w+depth*16u+slot*8u;
     if(!seen.insert((i<<8)|pathWord).second)continue;
     result.push_back({i,w,value,target,identity.readable?identity.candidateId:0,
       identity.readable?identity.candidateOwner:0,depth?"indirect":"direct",
       selected,owned});
    }
   }
  }
 }
 return result;
}
}