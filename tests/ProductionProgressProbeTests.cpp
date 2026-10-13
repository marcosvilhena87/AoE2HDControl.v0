#include "ProductionProgressProbe.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake:MemoryReader {
 std::map<uint32_t,unsigned char> bytes;
 void put(uint32_t a,uint32_t v){for(unsigned i=0;i<4;++i)bytes[a+i]=static_cast<unsigned char>(v>>(8*i));}
 bool read(uint32_t a,void* out,size_t n)const override {
  auto* b=static_cast<unsigned char*>(out);
  for(size_t i=0;i<n;++i){auto it=bytes.find(a+static_cast<uint32_t>(i));if(it==bytes.end())return false;b[i]=it->second;}
  return true;
 }
};
int main(){
 Fake m;
 UnitVectorCandidate c;c.source="World";c.offset=0xA0;c.stride=4;c.count=1;
 c.begin=0x1000;c.reason=UnitCandidateReason::Accepted;
 m.put(0x1000,0x2000);
 for(uint32_t i=0;i<64;++i)m.put(0x2000+i*4,0);
 float progress=0.5f;uint32_t bits=0;std::memcpy(&bits,&progress,4);
 m.put(0x2010,bits);m.put(0x2014,42);
 auto found=ScanProgressCandidates(m,{c});
 bool fraction=false,integer=false;
 for(const auto& f:found){if(f.fieldOffset==16 && f.encoding==std::string("fraction"))fraction=true;
 if(f.fieldOffset==20 && f.encoding==std::string("integer"))integer=true;}
 assert(fraction && integer);
 m.bytes.erase(0x2020);
 assert(ScanProgressCandidates(m,{c}).empty());
 std::cout<<"Production progress probe tests passed\n";
}
