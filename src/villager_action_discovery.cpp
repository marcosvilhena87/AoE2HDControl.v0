#include "ProcessMemory.h"
#include "SelectedTelemetryReader.h"
#include "VillagerActionDiscovery.h"
#include <conio.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
namespace {
struct Capture {
 aoe2hd::EntityTypeSample fields;
 uint32_t id=0,object=0,definition=0,typeA=0,typeB=0;
 bool typeReadable=false;
};
bool take(const aoe2hd::MemoryReader& mem,uint32_t moduleBase,
 uint32_t lockedId,uint32_t lockedObject,Capture& dst){
 const auto world=aoe2hd::ReadWorld(mem,moduleBase);
 const auto before=aoe2hd::ReadSelectedTelemetry(mem,world);
 if(before.status!=aoe2hd::TelemetryStatus::Ready||
 before.ownership!=aoe2hd::EntityOwnership::Local||
 before.identity.candidateId!=lockedId||before.object!=lockedObject){
  std::cout<<"Selection changed or unreadable; capture rejected.\n";return false;
 }
 auto fields=aoe2hd::SampleEntityType(mem,before.object,"entity",0x400u);
 const auto after=aoe2hd::ReadSelectedTelemetry(mem,aoe2hd::ReadWorld(mem,moduleBase));
 if(after.status!=aoe2hd::TelemetryStatus::Ready||
  after.object!=lockedObject||after.identity.candidateId!=lockedId||
  after.identity.candidateTypeRef!=before.identity.candidateTypeRef||
  after.ownership!=before.ownership){
  std::cout<<"Entity/definition changed during capture; rejected.\n";return false;
 }
 dst.fields=std::move(fields);dst.object=before.object;dst.id=lockedId;
 dst.definition=before.identity.candidateTypeRef;
 dst.typeA=before.typeCandidateA;dst.typeB=before.typeCandidateB;
 dst.typeReadable=before.typeCandidatesReadable;
 return true;
}
}
int main(int argc,char** argv){
 std::string prefix="aoe2hd_villager_action";
 if(argc==3&&std::string(argv[1])=="--prefix")prefix=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_villager_action_discovery [--prefix path]\n";return 2;}
 aoe2hd::ProcessMemory mem;
 if(!mem.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::cout<<"Select one LOCAL villager. Keys: 1 gather, 2 stop, 3 resume gather, q quit.\n"
 <<"Press each key in PowerShell AFTER observing the action in-game; these are manual labels.\n";
 uint32_t lockedId=0,lockedObject=0;
 std::vector<Capture> captures;
 bool done=false;
 while(!done){
  if(!lockedId){
   auto s=aoe2hd::ReadSelectedTelemetry(mem,aoe2hd::ReadWorld(mem,mem.moduleBase()));
   if(s.status==aoe2hd::TelemetryStatus::Ready&&s.ownership==aoe2hd::EntityOwnership::Local&&
     s.identity.candidateId){
    lockedId=s.identity.candidateId;lockedObject=s.object;
    std::cout<<"Locked ID "<<lockedId<<" object "<<lockedObject<<".\n";
   }
  }
  while(_kbhit()){
   int k=_getch();
   if(k=='q'||k=='Q'){done=true;break;}
   if(k<'1'||k>'3')continue;
   const size_t expected=captures.size()+1;
   if(!lockedId||static_cast<size_t>(k-'0')!=expected){
    std::cout<<"Expected phase "<<expected<<" after locking a local villager.\n";continue;
   }
   Capture s;
   if(take(mem,mem.moduleBase(),lockedId,lockedObject,s)){
    captures.push_back(std::move(s));
    std::cout<<"Captured phase "<<captures.size()<<" (definition "<<captures.back().definition<<").\n";
   }
   if(captures.size()==3)done=true;
  }
  if(!done)std::this_thread::sleep_for(std::chrono::milliseconds(100));
 }
 if(captures.size()!=3){
  std::cout<<"Incomplete triplet; no comparison produced.\n";return 1;
 }
 std::ofstream raw(prefix+"_fields.csv",std::ios::trunc),results(prefix+"_candidates.csv",std::ios::trunc);
 if(!raw||!results){std::cerr<<"Could not create CSV files\n";return 2;}
 raw<<"phase,entity_id,object,definition_ref,type_a,type_b,type_readable,offset,readable,raw_u32\n";
 const char* phases[]={"gathering_1","stopped","gathering_2"};
 for(size_t i=0;i<3;++i){
  const auto& s=captures[i];
  for(const auto& field:s.fields.fields)
   raw<<phases[i]<<','<<s.id<<','<<s.object<<','<<s.definition<<','
    <<s.typeA<<','<<s.typeB<<','<<s.typeReadable<<','
    <<field.offset<<','<<field.readable<<','<<field.raw<<'\n';
 }
 results<<"offset,all_readable,gathering_1,stopped,gathering_2,changed_on_stop,reversed_on_resume\n";
 unsigned count=0;
 for(const auto& v:aoe2hd::CompareActionTriplet(
   captures[0].fields,captures[1].fields,captures[2].fields)){
  results<<v.offset<<','<<v.readable<<','<<v.gathering1<<','<<v.stopped<<','
   <<v.gathering2<<','<<v.changed<<','<<v.reverses<<'\n';
  if(v.reverses)++count;
 }
 std::cout<<"Saved "<<prefix<<"_fields.csv and "<<prefix<<"_candidates.csv; "
 <<count<<" reversible offsets (not confirmed action fields).\n";
 if(captures[0].definition!=captures[1].definition||
  captures[1].definition!=captures[2].definition)
  std::cout<<"WARNING: definition changed between phases; interpret candidate offsets cautiously.\n";
 return 0;
}
