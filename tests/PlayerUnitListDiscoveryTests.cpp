#include "PlayerUnitListDiscovery.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
struct Mock:aoe2hd::MemoryReader {
 std::map<uint32_t,uint8_t> bytes;
 void put(uint32_t a,uint32_t v){for(int i=0;i<4;++i)bytes[a+i]=static_cast<uint8_t>(v>>(i*8));}
 bool read(uint32_t a,void* p,size_t n)const override{
  auto* b=static_cast<uint8_t*>(p);
  for(size_t i=0;i<n;++i){auto it=bytes.find(a+static_cast<uint32_t>(i));if(it==bytes.end())return false;b[i]=it->second;}
  return true;
 }
};
int main(){
 Mock m;const uint32_t local=0x20000,other=0x21000;
 aoe2hd::WorldResult w;w.status=aoe2hd::WorldStatus::Ready;
 w.snapshot.localIndex=0;w.snapshot.players.resize(2);
 w.snapshot.players[0].object=local;w.snapshot.players[1].object=other;
 const uint32_t arr=0x30000,a=0x40000,b=0x41000,c=0x42000;
 for(int i=0;i<3;++i)m.put(arr+4*i,i==0?a:(i==1?b:c));
 const uint32_t objs[]={a,b,c};
 const uint32_t owners[]={local,local,other};
 for(int i=0;i<3;++i){
  for(int j=0;j<5;++j)m.put(objs[i]+4*j,0);
  m.put(objs[i]+8,2200+i);m.put(objs[i]+16,owners[i]);
 }
 aoe2hd::UnitVectorCandidate candidate;
 candidate.source="LocalPlayer";candidate.begin=arr;candidate.count=3;
 candidate.stride=4;candidate.reason=aoe2hd::UnitCandidateReason::Accepted;
 auto v=aoe2hd::ProbePlayerUnitLists(m,w,{candidate},b);
 assert(v.size()==1&&v[0].sampled==3&&v[0].localMatches==2);
 assert(v[0].otherMatches==1&&v[0].selectedMatches==1);
 assert(v[0].sampledIds.size()==2&&v[0].sampledIds[0]==2200);
 assert(aoe2hd::ProbePlayerUnitLists(m,w,{},b).empty());
 std::cout<<"Player unit list discovery tests passed\n";
}