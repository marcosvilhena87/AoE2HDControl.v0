#include "UnitCollectionDiscovery.h"
#include <cassert>
#include <cstdint>
#include <map>
struct Fake:aoe2hd::MemoryReader{
 std::map<uint32_t,uint8_t> bytes;
 void put(uint32_t a,uint32_t v){
  for(unsigned i=0;i<4;++i)bytes[a+i]=static_cast<uint8_t>(v>>(8*i));
 }
 bool read(uint32_t a,void* out,size_t size)const override{
  auto* p=static_cast<uint8_t*>(out);
  for(size_t i=0;i<size;++i){
   auto it=bytes.find(a+static_cast<uint32_t>(i));
   if(it==bytes.end())return false;
   p[i]=it->second;
  }
  return true;
 }
};
int main(){
 Fake memory;
 aoe2hd::WorldResult world;
 world.status=aoe2hd::WorldStatus::Ready;
 world.snapshot.world=0x10000;
 world.snapshot.localIndex=0;
 world.snapshot.players.resize(1);
 world.snapshot.players[0].object=0x20000;
 const uint32_t begin=0x30000,unit=0x40000;
 // Scan window holds a plausible pointer triple at LocalPlayer+0x20.
 memory.put(0x20020,begin);memory.put(0x20024,begin+4);
 memory.put(0x20028,begin+4);memory.put(begin,unit);
 for(unsigned i=0;i<5;++i)memory.put(unit+4*i,0);
 memory.put(unit+8,2234);memory.put(unit+16,0x20000);
 auto result=aoe2hd::InspectUnitCollections(memory,world,unit,0x40,16,16);
 assert(result.observations.size()==1);
 const auto& o=result.observations[0];
 assert(o.summary.selectedMatches==1);
 assert(o.summary.localMatches==1);
 assert(!o.entries.empty());
 assert(o.summary.vector.begin==begin);
 assert(aoe2hd::InspectUnitCollections(memory,aoe2hd::WorldResult{}).observations.empty());
}
