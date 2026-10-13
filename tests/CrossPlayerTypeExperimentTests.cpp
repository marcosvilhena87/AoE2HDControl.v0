#include "CrossPlayerTypeExperiment.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake : MemoryReader {
 std::map<uint32_t,uint32_t> words;
 bool read(uint32_t a,void* dst,size_t n)const override {
  if(n==4){auto i=words.find(a);if(i==words.end())return false;std::memcpy(dst,&i->second,4);return true;}
  if(n==20){uint32_t data[5];
   for(unsigned i=0;i<5;++i){auto j=words.find(a+4*i);if(j==words.end())return false;data[i]=j->second;}
   std::memcpy(dst,data,20);return true;}
  return false;
 }
 void entity(uint32_t obj,uint32_t definition,uint32_t owner,uint32_t a,uint32_t b){
  for(int i=0;i<5;++i)words[obj+4*i]=0;
  words[obj+12]=definition;words[obj+16]=owner;
  words[definition]=0;words[definition+4]=0;
  words[definition+8]=a;words[definition+12]=b;
 }
};
int main(){
 Fake m;WorldResult w;w.status=WorldStatus::Ready;w.snapshot.localIndex=1;
 w.snapshot.players.resize(3);w.snapshot.players[1].object=0x20000;w.snapshot.players[2].object=0x30000;
 m.entity(0x40000,0x60000,0x20000,5326,6326);
 m.entity(0x41000,0x61000,0x30000,5326,6326);
 auto a=CaptureCrossPlayerType(m,w,"scout_local","scout",0x40000);
 auto b=CaptureCrossPlayerType(m,w,"scout_other","scout",0x41000);
 assert(a.ownership==EntityOwnership::Local);
 assert(b.ownership==EntityOwnership::OtherPlayer);
 assert(CompareCrossPlayerType(a,b,false)==PairVerdict::Equal);
 assert(CompareCrossPlayerType(a,b,true)==PairVerdict::Equal);
 m.words[0x6100c]=999;
 b=CaptureCrossPlayerType(m,w,"scout_other","scout",0x41000);
 assert(CompareCrossPlayerType(a,b,true)==PairVerdict::Different);
 m.words.erase(0x61008);
 b=CaptureCrossPlayerType(m,w,"scout_other","scout",0x41000);
 assert(CompareCrossPlayerType(a,b,false)==PairVerdict::Insufficient);
 std::cout<<"Cross-player type experiment tests passed\n";
}
