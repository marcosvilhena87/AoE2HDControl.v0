#include "ConstructionPointerDiscovery.h"
#include <algorithm>
#include <set>
namespace aoe2hd {
std::vector<ConstructionPointerHit> FindConstructionPointerReferences(
 const MemoryReader& memory,const WorldResult& world,uint32_t target,
 uint32_t scanBytes,uint32_t maxIntermediates){
 std::vector<ConstructionPointerHit> hits;
 if(!world.ready()||target<0x10000u||(target&3))return hits;
 scanBytes=std::min(scanBytes,0x400u)&~3u;
 maxIntermediates=std::min(maxIntermediates,64u);
 const auto* player=GetLocalPlayer(world);
 if(!player)return hits;
 const struct {const char* name;uint32_t base;} roots[]={
   {"World",world.snapshot.world},{"LocalPlayer",player->object}};
 std::set<uint32_t> inspected;
 for(const auto& root:roots){
  if(root.base<0x10000u)continue;
  for(uint32_t offset=0;offset<scanBytes;offset+=4){
   if(root.base>UINT32_MAX-offset)break;
   uint32_t word=0;
   if(!memory.read(root.base+offset,&word,4))continue;
   if(word==target)hits.push_back({root.name,offset,0,0,target,1});
   if(word<0x10000u||(word&3)||word>UINT32_MAX-scanBytes ||
       word==root.base||word==player->object||word==world.snapshot.world ||
       inspected.count(word)||inspected.size()>=maxIntermediates)continue;
   inspected.insert(word);
   for(uint32_t inner=0;inner<scanBytes;inner+=4){
    uint32_t nested=0;
    if(memory.read(word+inner,&nested,4)&&nested==target)
     hits.push_back({root.name,offset,word,inner,target,2});
   }
  }
 }
 return hits;
}
}
