#include "ConstructionProgressMonitor.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake:MemoryReader {
 std::map<uint32_t,uint32_t> words;
 bool read(uint32_t a,void* p,size_t n)const override {
  auto i=words.find(a);
  if(i==words.end()||n!=4)return false;
  std::memcpy(p,&i->second,4);return true;
 }
};
int main(){
 Fake f;ConstructionProgressMonitor m;
 const uint32_t base=0x70000,field=base+ConstructionProgressFieldOffset;
 float v=78.0f;uint32_t raw=0;std::memcpy(&raw,&v,4);f.words[field]=raw;
 auto s=m.observe(f,{base});
 assert(s.size()==1&&s[0].address==field&&s[0].value==78.0f&&s[0].inRange);
 assert(m.observe(f,{base}).empty());
 v=79.0f;std::memcpy(&raw,&v,4);f.words[field]=raw;
 s=m.observe(f,{base});
 assert(s.size()==1&&s[0].kind=="change"&&s[0].value==79.0f);
 s=m.observe(f,{base},true);assert(s.size()==1&&s[0].kind=="phase");
 f.words.erase(field);s=m.observe(f,{base});
 assert(s.size()==1&&s[0].kind=="status"&&!s[0].readable);
 s=m.observe(f,{0xFFFFF800u});
 assert(s.size()==1&&!s[0].readable);
 m.reset();s=m.observe(f,{base});
 assert(s.size()==1&&s[0].kind=="initial");
 std::cout<<"Construction progress monitor tests passed\n";
}
