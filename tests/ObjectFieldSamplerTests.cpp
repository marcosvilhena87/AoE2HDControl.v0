#include "ObjectFieldSampler.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
using namespace aoe2hd;
struct Fake:MemoryReader {
 std::map<uint32_t,uint32_t> words;
 bool read(uint32_t a,void* dst,size_t n)const override {
  auto it=words.find(a);if(n!=4||it==words.end())return false;
  std::memcpy(dst,&it->second,4);return true;
 }
};
int main() {
 Fake memory;
 UnitVectorCandidate c;c.source="World";c.offset=0x88;c.stride=8;c.count=1;
 c.begin=0x4000;c.reason=UnitCandidateReason::Accepted;
 memory.words[0x4000]=0x5000;
 for(uint32_t i=0;i<16;++i)memory.words[0x5000+4*i]=i;
 ObjectFieldSampler sampler;
 assert(sampler.observe(memory,{c}).empty());
 memory.words[0x500c]=99;
 auto changes=sampler.observe(memory,{c});
 assert(changes.size()==1 && changes[0].field==12 && changes[0].after==99);
 memory.words[0x4000]=0x6000;
 for(uint32_t i=0;i<16;++i)memory.words[0x6000+4*i]=i;
 changes=sampler.observe(memory,{c});
 assert(changes.size()==1 && changes[0].kind=="object_changed");
 sampler.reset();
 assert(sampler.observe(memory,{c}).empty());
 memory.words.erase(0x6008);
 assert(sampler.observe(memory,{c}).empty());
 std::cout<<"Object field sampler tests passed\n";
}
