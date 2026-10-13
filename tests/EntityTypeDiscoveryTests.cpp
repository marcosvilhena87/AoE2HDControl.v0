#include "EntityTypeDiscovery.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
using namespace aoe2hd;
struct Fake:MemoryReader {
 std::map<uint32_t,uint32_t> words;
 bool read(uint32_t a,void* out,size_t n) const override{
  auto it=words.find(a);if(n!=4||it==words.end())return false;
  std::memcpy(out,&it->second,4);return true;
 }
};
int main(){
 Fake f;
 f.words[0x10000]=0x8888;f.words[0x20000]=0x9999;
 f.words[0x10004]=25;f.words[0x20004]=25;
 auto a=SampleEntityType(f,0x10000,"villager",12);
 auto b=SampleEntityType(f,0x20000,"house",12);
 auto rows=CompareEntityTypes({a,b});
 assert(rows.size()==3&&rows[0].differentiates&&rows[0].distinctValues==2);
 assert(rows[1].allReadable&&!rows[1].differentiates);
 assert(!rows[2].allReadable);
 assert(SampleEntityType(f,0x10000,"x",0xFFFF).fields.size()==256);
 std::cout<<"Entity type discovery tests passed\n";
}
