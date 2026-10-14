#include "ProcessReferenceScanner.h"
#include <cstring>
#include <map>
namespace aoe2hd {
std::vector<MultiReferenceHit> ScanReferenceBlock(uint32_t base,const uint8_t* data,
 uint32_t bytes,const std::vector<ScanAnchor>& anchors,uint32_t maxHits){
 std::vector<MultiReferenceHit> hits;
 if(!data||bytes<4||!maxHits)return hits;
 std::map<uint32_t,uint32_t> targets;
 for(const auto& a:anchors)if(a.object&&a.id)targets[a.object]=a.id;
 for(uint32_t n=0;n+4<=bytes&&hits.size()<maxHits;n+=4){
  uint32_t value=0;
  std::memcpy(&value,data+n,4);
  auto it=targets.find(value);
  if(it!=targets.end()&&base<=UINT32_MAX-n)
   hits.push_back({base+n,it->second,value});
 }
 return hits;
}
}