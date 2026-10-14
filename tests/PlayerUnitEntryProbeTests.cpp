#include "PlayerUnitEntryProbe.h"
#include <cassert>
#include <map>
#include <string>
struct Mock:aoe2hd::MemoryReader{
 std::map<uint32_t,uint8_t> b;
 void put(uint32_t a,uint32_t v){for(int i=0;i<4;++i)b[a+i]=static_cast<uint8_t>(v>>(i*8));}
 bool read(uint32_t a,void* dst,size_t n)const override{
  auto* p=static_cast<uint8_t*>(dst);for(size_t i=0;i<n;++i){auto x=b.find(a+static_cast<uint32_t>(i));if(x==b.end())return false;p[i]=x->second;}return true;
 }
};
int main(){
 Mock m;aoe2hd::WorldResult w;w.status=aoe2hd::WorldStatus::Ready;
 w.snapshot.localIndex=0;w.snapshot.players.resize(1);w.snapshot.players[0].object=0x21000;
 for(int j=0;j<5;++j)m.put(0x40000+j*4,0);
 m.put(0x40008,2547);m.put(0x40010,0x21000);
 m.put(0x30000,0x40000);m.put(0x30004,0);
 m.put(0x30008,0x50000);m.put(0x3000c,42);
 m.put(0x50000,0x40000);m.put(0x50004,0);
 aoe2hd::UnitVectorCandidate c;c.begin=0x30000;c.count=2;c.stride=8;
 auto e=aoe2hd::ProbeUnitEntryEvidence(m,w,c,0x40000);
 bool direct=false,indirect=false;
 for(auto x:e){if(x.kind==std::string("direct")&&x.localOwner)direct=true;
 if(x.kind==std::string("indirect")&&x.localOwner)indirect=true;}
 assert(direct&&indirect);
 c.count=1025;assert(aoe2hd::ProbeUnitEntryEvidence(m,w,c).empty());
}