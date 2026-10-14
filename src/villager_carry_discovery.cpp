#include "VillagerCarryDiscovery.h"
#include "SelectedTelemetryReader.h"
#include "ProcessMemory.h"
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
namespace {
struct Capture {
 aoe2hd::CarryObservation observation;
 uint32_t id=0,object=0,definition=0,typeA=0;
 std::string event;
};
bool take(const aoe2hd::ProcessMemory& mem, uint32_t id,uint32_t object,
 const std::string& event,float hud,bool stopped,Capture& dst){
 auto before=aoe2hd::ReadSelectedTelemetry(mem,aoe2hd::ReadWorld(mem,mem.moduleBase()));
 if(before.status!=aoe2hd::TelemetryStatus::Ready||
    before.ownership!=aoe2hd::EntityOwnership::Local||
    before.identity.candidateId!=id||before.object!=object){
  std::cout<<"Rejected: selected entity changed or unreadable.\n";return false;
 }
 auto fields=aoe2hd::SampleEntityType(mem,object,event,0x400);
 auto after=aoe2hd::ReadSelectedTelemetry(mem,aoe2hd::ReadWorld(mem,mem.moduleBase()));
 if(after.status!=aoe2hd::TelemetryStatus::Ready||
    after.identity.candidateId!=id||after.object!=object||
    after.identity.candidateTypeRef!=before.identity.candidateTypeRef||
    after.ownership!=before.ownership){
  std::cout<<"Rejected: identity/definition changed during capture.\n";return false;
 }
 dst.observation={hud,stopped,std::move(fields)};
 dst.id=id;dst.object=object;dst.definition=before.identity.candidateTypeRef;
 dst.typeA=before.typeCandidateA;dst.event=event;
 return true;
}
}
int main(int argc,char** argv){
 std::string prefix="aoe2hd_villager_carry";
 if(argc==3&&std::string(argv[1])=="--prefix")prefix=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_villager_carry_discovery [--prefix path]\n";return 2;}
 aoe2hd::ProcessMemory mem;
 if(!mem.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::cout<<"Read-only carry field experiment. Select ONE local villager before capture.\n"
 <<"Observe HUD load, then enter: event load stop_flag (0 or 1).\n"
 <<"Events: gathering, stopped, resumed, deposited, reassigned. Example: stopped 5 1\n"
 <<"Type q to save. IMPORTANT: keep the SAME villager selected throughout.\n";
 uint32_t id=0,object=0;
 std::vector<Capture> captures;
 std::string line;
 while(std::getline(std::cin,line)){
  if(line=="q"||line=="Q")break;
  std::istringstream input(line);
  std::string event,extra;float load=0;int stop=0;
  if(!(input>>event>>load>>stop)||(input>>extra)||
     (event!="gathering"&&event!="stopped"&&event!="resumed"&&
      event!="deposited"&&event!="reassigned")||
      !std::isfinite(load)||load<0||load>10||stop<0||stop>1||
      (event=="stopped")!=(stop==1)){
   std::cout<<"Invalid: enter gathering|stopped|resumed|deposited|reassigned LOAD(0..10) STOP(0|1).\n";
   continue;
  }
  if(!id){
   auto s=aoe2hd::ReadSelectedTelemetry(mem,aoe2hd::ReadWorld(mem,mem.moduleBase()));
   if(s.status!=aoe2hd::TelemetryStatus::Ready||
      s.ownership!=aoe2hd::EntityOwnership::Local||
      !s.identity.candidateId){std::cout<<"Select one local villager first.\n";continue;}
   id=s.identity.candidateId;object=s.object;
   std::cout<<"Locked id="<<id<<" object="<<object<<'\n';
  }
  Capture sample;
  if(!take(mem,id,object,event,load,stop==1,sample))continue;
  captures.push_back(std::move(sample));
  std::cout<<"Captured "<<event<<" HUD="<<load<<" id="<<id
           <<" typeA="<<captures.back().typeA<<"; total="<<captures.size()<<'\n';
 }
 std::ofstream raw(prefix+"_fields.csv"),rank(prefix+"_candidates.csv");
 if(!raw||!rank){std::cerr<<"Cannot write CSV\n";return 2;}
 raw<<"sample,event,hud_load,stopped,id,object,definition,type_a,offset,readable,raw_u32,as_float\n";
 std::vector<aoe2hd::CarryObservation> observations;
 for(size_t i=0;i<captures.size();++i){
  const auto& c=captures[i];observations.push_back(c.observation);
  for(const auto& f:c.observation.sample.fields){
   float value=0;std::memcpy(&value,&f.raw,4);
   raw<<i<<','<<c.event<<','<<c.observation.hudLoad<<','<<c.observation.stopped
      <<','<<c.id<<','<<c.object<<','<<c.definition<<','<<c.typeA<<','
      <<f.offset<<','<<f.readable<<','<<f.raw<<',';
   if(f.readable&&std::isfinite(value))raw<<std::setprecision(9)<<value;
   raw<<'\n';
  }
 }
 rank<<"offset,matched_hud,total,stopped_pairs,preserved_stopped_pairs,perfect_candidate\n";
 auto candidates=aoe2hd::RankCarryFields(observations);
 for(const auto& x:candidates)
  rank<<x.offset<<','<<x.matched<<','<<x.total<<','<<x.stoppedPairs<<','
      <<x.preservedStopPairs<<','<<x.perfect<<'\n';
 std::cout<<"Wrote "<<captures.size()<<" snapshots and "<<candidates.size()
          <<" readable candidate offsets. No automatic carry field is confirmed.\n";
 return 0;
}
