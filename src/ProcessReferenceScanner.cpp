#include "ProcessReferenceScanner.h"
#include <cstring>
namespace aoe2hd {
ReferenceLookup BuildReferenceLookup(const std::vector<ScanAnchor>& anchors){
 ReferenceLookup lookup;
 for(const auto& a:anchors)if(a.object&&a.id)lookup[a.object]=a.id;
 return lookup;
}
bool MatchReferenceWord(const ReferenceLookup& lookup,uint32_t value,uint32_t& id){
 const auto it=lookup.find(value);
 if(it==lookup.end())return false;
 id=it->second;
 return true;
}
std::vector<MultiReferenceHit> ScanReferenceBlock(uint32_t base,const uint8_t* data,
 uint32_t bytes,const std::vector<ScanAnchor>& anchors,uint32_t maxHits){
 std::vector<MultiReferenceHit> hits;
 if(!data||bytes<4||!maxHits)return hits;
 const auto lookup=BuildReferenceLookup(anchors);
 for(uint32_t n=0;n+4<=bytes&&hits.size()<maxHits;n+=4){
  uint32_t value=0,id=0;
  std::memcpy(&value,data+n,4);
  if(MatchReferenceWord(lookup,value,id)&&base<=UINT32_MAX-n)
   hits.push_back({base+n,id,value});
 }
 return hits;
}
}