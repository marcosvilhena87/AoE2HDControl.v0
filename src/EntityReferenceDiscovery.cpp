#include "EntityReferenceDiscovery.h"
#include "ProcessReferenceScanner.h"
namespace aoe2hd {
std::vector<ReferenceHit> FindExactReferences(const MemoryReader& mem,
 uint32_t begin,uint32_t bytes,uint32_t anchor,uint32_t maxHits){
 std::vector<ReferenceHit> hits;
 if(!anchor||bytes<4||!maxHits||begin>UINT32_MAX-bytes)return hits;
 // Preserve four-byte reads to handle partially unreadable intervals.
 const auto lookup=BuildReferenceLookup({ScanAnchor{1,anchor}});
 for(uint32_t offset=0;offset<=bytes-4&&hits.size()<maxHits;offset+=4){
  uint32_t value=0,id=0;
  if(mem.read(begin+offset,&value,4)&&MatchReferenceWord(lookup,value,id))
   hits.push_back({begin+offset,value});
 }
 return hits;
}
}