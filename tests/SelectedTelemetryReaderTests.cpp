#include "SelectedTelemetryReader.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake : MemoryReader {
 std::map<uint32_t,uint32_t> values;
 bool read(uint32_t addr,void* dst,size_t size) const override{
  if(size==4){auto it=values.find(addr);if(it==values.end())return false;
   std::memcpy(dst,&it->second,4);return true;}
  if(size==20){uint32_t data[5];
   for(int i=0;i<5;++i){auto it=values.find(addr+i*4);if(it==values.end())return false;data[i]=it->second;}
   std::memcpy(dst,data,20);return true;}
  return false;
 }
};
int main(){
 Fake f;WorldResult w;w.status=WorldStatus::Ready;w.snapshot.localIndex=1;
 w.snapshot.players.resize(3);w.snapshot.players[1].object=0x20000;
 w.snapshot.players[2].object=0x30000;
 assert(ReadSelectedTelemetry(f,w).status==TelemetryStatus::SlotReadFailed);
 f.values[0x2015c]=0;f.values[0x20160]=0;
 assert(ReadSelectedTelemetry(f,w).status==TelemetryStatus::None);
 f.values[0x2015c]=0x40000;f.values[0x20160]=0x50000;
 assert(ReadSelectedTelemetry(f,w).status==TelemetryStatus::MultipleOrTransition);
 f.values[0x20160]=0x40000;
 for(int i=0;i<5;i++)f.values[0x40000+i*4]=0;
 f.values[0x40008]=123;f.values[0x40010]=0x20000;
 float hp=45;uint32_t raw=0;std::memcpy(&raw,&hp,4);f.values[0x40034]=raw;
 auto a=ReadSelectedTelemetry(f,w);
 assert(a.status==TelemetryStatus::Ready&&a.hp==45&&a.identity.candidateId==123&&a.ownership==EntityOwnership::Local);
 f.values[0x40010]=0x30000;
 assert(ReadSelectedTelemetry(f,w).ownership==EntityOwnership::OtherPlayer);
 f.values.erase(0x40034);
 assert(ReadSelectedTelemetry(f,w).status==TelemetryStatus::HpReadFailed);
 w.status=WorldStatus::MissingWorld;
 assert(ReadSelectedTelemetry(f,w).status==TelemetryStatus::WorldUnavailable);
 std::cout<<"Selected telemetry tests passed\n";
}