#include "IdleVillagerDiscovery.h"
#include "SelectedTelemetryReader.h"
#include "ProcessMemory.h"
#include <conio.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
namespace {
struct Record {
 aoe2hd::IdleObservation observation;
 uint32_t entityId=0,object=0,definition=0,typeA=0;
 bool typeReadable=false;
 const char* source="working";
};
bool capture(const aoe2hd::ProcessMemory& mem,bool idleButton,Record& out){
 const auto before=aoe2hd::ReadSelectedTelemetry(mem,aoe2hd::ReadWorld(mem,mem.moduleBase()));
 if(before.status!=aoe2hd::TelemetryStatus::Ready||
    before.ownership!=aoe2hd::EntityOwnership::Local||
    !before.identity.candidateId){std::cout<<"No valid single local entity selected.\n";return false;}
 auto fields=aoe2hd::SampleEntityType(mem,before.object,"entity",0x400);
 const auto after=aoe2hd::ReadSelectedTelemetry(mem,aoe2hd::ReadWorld(mem,mem.moduleBase()));
 if(after.status!=aoe2hd::TelemetryStatus::Ready||
    after.object!=before.object||
    after.identity.candidateId!=before.identity.candidateId||
    after.identity.candidateTypeRef!=before.identity.candidateTypeRef||
    after.identity.candidateOwner!=before.identity.candidateOwner){
  std::cout<<"Selection/definition changed during capture; rejected.\n";return false;
 }
 out.observation={idleButton,std::move(fields)};
 out.entityId=before.identity.candidateId;out.object=before.object;
 out.definition=before.identity.candidateTypeRef;
 out.typeA=before.typeCandidateA;out.typeReadable=before.typeCandidatesReadable;
 out.source=idleButton?"idle_button":"working";
 return true;
}
}
int main(int argc,char** argv){
 std::string prefix="aoe2hd_idle_villager";
 if(argc==3&&std::string(argv[1])=="--prefix")prefix=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_idle_villager_discovery [--prefix path]\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::cout<<"Read-only: select a known working villager, focus console and press w.\n"
 <<"For idle: click the game's Next Idle Villager HUD button, focus console and press i.\n"
 <<"Use s only for a manually stopped villager (separate category). Press q to save.\n";
 std::vector<Record> records;
 bool done=false;
 while(!done){
  while(_kbhit()){
   int key=_getch();
   if(key=='q'||key=='Q'){done=true;break;}
   if(key!='w'&&key!='i'&&key!='s')continue;
   Record record;
   if(!capture(memory,key=='i',record))continue;
   if(key=='s')record.source="stopped_manual";
   std::cout<<"Captured "<<record.source<<" id="<<record.entityId
    <<" object="<<record.object<<" typeA="<<record.typeA<<'\n';
   records.push_back(std::move(record));
  }
  if(!done)std::this_thread::sleep_for(std::chrono::milliseconds(100));
 }
 std::ofstream raw(prefix+"_fields.csv",std::ios::trunc),comparison(prefix+"_candidates.csv",std::ios::trunc);
 if(!raw||!comparison){std::cerr<<"Cannot create CSV files\n";return 2;}
 raw<<"sample,source,entity_id,object,definition,type_a,type_readable,offset,readable,raw_u32\n";
 std::vector<aoe2hd::IdleObservation> eligible;
 for(size_t i=0;i<records.size();++i){
  const auto& s=records[i];
  if(std::string(s.source)!="stopped_manual")eligible.push_back(s.observation);
  for(const auto& f:s.observation.fields.fields)
   raw<<i<<','<<s.source<<','<<s.entityId<<','<<s.object<<','
     <<s.definition<<','<<s.typeA<<','<<s.typeReadable<<','
     <<f.offset<<','<<f.readable<<','<<f.raw<<'\n';
 }
 comparison<<"offset,all_readable,working_samples,idle_button_samples,working_distinct,idle_distinct,disjoint_values\n";
 unsigned candidates=0;
 for(const auto& e:aoe2hd::CompareIdleObservations(eligible)){
  comparison<<e.offset<<','<<e.allReadable<<','<<e.workingCount<<','
   <<e.idleCount<<','<<e.workingDistinct<<','<<e.idleDistinct<<','<<e.disjoint<<'\n';
  if(e.disjoint)++candidates;
 }
 std::cout<<"Saved "<<records.size()<<" observations; "<<candidates
 <<" offsets with disjoint observed values (NOT confirmed idle flags).\n";
 return 0;
}
