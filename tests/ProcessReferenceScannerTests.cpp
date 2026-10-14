#include "ProcessReferenceScanner.h"
#include <cassert>
#include <cstring>
int main(){
 uint32_t words[]={41,0x12340000,0x12340004,0x12340000,0};
 auto a=std::vector<aoe2hd::ScanAnchor>{{2234,0x12340000},{2235,0x12340004}};
 auto h=aoe2hd::ScanReferenceBlock(0x10000,
  reinterpret_cast<const uint8_t*>(words),sizeof(words),a);
 assert(h.size()==3&&h[0].address==0x10004&&h[1].id==2235);
 assert(aoe2hd::ScanReferenceBlock(0,nullptr,20,a).empty());
 assert(aoe2hd::ScanReferenceBlock(0,reinterpret_cast<const uint8_t*>(words),3,a).empty());
 assert(aoe2hd::ScanReferenceBlock(0x10000,reinterpret_cast<const uint8_t*>(words),sizeof(words),a,1).size()==1);
 auto lookup=aoe2hd::BuildReferenceLookup({{0,0x1000},{4,0},{7,0x2000},{8,0x2000}});
 uint32_t id=0;
 assert(lookup.size()==1);
 assert(!aoe2hd::MatchReferenceWord(lookup,0x1000,id));
 assert(aoe2hd::MatchReferenceWord(lookup,0x2000,id)&&id==8);
 assert(!aoe2hd::MatchReferenceWord(lookup,0x3000,id));
}