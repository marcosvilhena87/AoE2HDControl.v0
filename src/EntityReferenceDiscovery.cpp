#include "EntityReferenceDiscovery.h"
#include <algorithm>
#include <cstring>
namespace aoe2hd {
std::vector<ReferenceHit> FindExactReferences(const MemoryReader& mem,
 uint32_t begin,uint32_t bytes,uint32_t anchor,uint32_t maxHits){
 std::vector<ReferenceHit> hits;
 if(!anchor||!bytes||!maxHits||begin>UINT32_MAX-bytes)return hits;
 // Scan dword-aligned slots; non-readable pages are skipped one dword at a time.
 for(uint32_t offset=0;offset<=bytes-4&&hits.size()<maxHits;offset+=4){
  uint32_t value=0;
  if(mem.read(begin+offset,&value,4)&&value==anchor)hits.push_back({begin+offset,anchor});
 }
 return hits;
}
}