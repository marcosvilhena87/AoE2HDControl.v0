#include "ConstructionPointerDiscovery.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake:MemoryReader {
 std::map<uint32_t,uint32_t> words;
 bool read(uint32_t a,void* dst,size_t n)const override {
  auto it=words.find(a);if(it==words.end()||n!=4)return false;
  std::memcpy(dst,&it->second,4);return true;
 }
};
int main(){
 Fake m;WorldResult w;w.status=WorldStatus::Ready;
 w.snapshot.world=0x20000;w.snapshot.localIndex=1;
 PlayerSnapshot gaia;gaia.object=0x21000;w.snapshot.players.push_back(gaia);
 PlayerSnapshot p;p.object=0x30000;w.snapshot.players.push_back(p);
 const uint32_t target=0x50000;
 m.words[0x20000+0x18]=target;
 m.words[0x30000+0x20]=0x40000;
 m.words[0x40000+0x28]=target;
 auto hits=FindConstructionPointerReferences(m,w,target);
 bool direct=false,indirect=false;
 for(const auto& h:hits){
  if(h.depth==1 && h.root=="World" && h.rootOffset==0x18)direct=true;
  if(h.depth==2 && h.root=="LocalPlayer" && h.rootOffset==0x20 &&
     h.intermediate==0x40000 && h.intermediateOffset==0x28)indirect=true;
 }
 assert(direct&&indirect);
 w.status=WorldStatus::InvalidWorldVtable;
 assert(FindConstructionPointerReferences(m,w,target).empty());
 std::cout<<"Construction pointer discovery tests passed\n";
}
