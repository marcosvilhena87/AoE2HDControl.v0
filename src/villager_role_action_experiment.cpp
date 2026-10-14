#include "SelectedTelemetryReader.h"
#include "VillagerObservationLabels.h"
#include "ProcessMemory.h"
#include <chrono>
#include <conio.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
namespace {
struct State {
 uint32_t object=0,id=0,typeRef=0,a=0,b=0;
 bool readable=false,stable=false;
 aoe2hd::TelemetryStatus status=aoe2hd::TelemetryStatus::WorldUnavailable;
 aoe2hd::EntityOwnership owner=aoe2hd::EntityOwnership::Unknown;
};
State stateOf(const aoe2hd::SelectedTelemetry& s){
 State st;st.object=s.object;st.id=s.identity.candidateId;
 st.typeRef=s.identity.candidateTypeRef;st.a=s.typeCandidateA;st.b=s.typeCandidateB;
 st.readable=s.typeCandidatesReadable;st.stable=s.typeReferenceStable;
 st.status=s.status;st.owner=s.ownership;return st;
}
bool differs(const State& a,const State& b){
 return a.object!=b.object||a.id!=b.id||a.typeRef!=b.typeRef||
 a.a!=b.a||a.b!=b.b||a.readable!=b.readable||
 a.stable!=b.stable||a.status!=b.status||a.owner!=b.owner;
}
}
int main(int argc,char** argv){
 std::string filename="aoe2hd_villager_role_action.csv";
 if(argc==3&&std::string(argv[1])=="--csv")filename=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_villager_role_action_experiment [--csv path]\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream out(filename,std::ios::trunc);
 if(!out){std::cerr<<"Cannot write CSV\n";return 2;}
 out<<"elapsed_ms,event,hud_role_observed,action_observed,locked_entity_id,object,entity_id,owner,status,definition_ref,type_ref_stable,type_candidates_readable,type_a,type_b,hp\n";
 std::cout<<"Select ONE villager first; first valid selection locks entity ID.\n"
          <<"HUD ROLE 0 villager, 1 gatherer, 2 shepherd, 3 hunter, 4 fisher, 5 gold miner, 6 lumberjack, 7 stone miner, 8 farmer, 9 builder.\n"
          <<"ACTION s stopped, m moving, g gathering, b building, u attack unit, e attack building, w attack wolf, x other.\n"
          <<"q quit.\n"
          <<"Mark HUD role and action SEPARATELY in console after observing them in-game; labels are not inferred.\n";
 const auto started=std::chrono::steady_clock::now();
 uint32_t lockedId=0;std::string hudRole="unmarked", action="unmarked";
 State previous;bool first=true,done=false;
 while(!done){
  const auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
  const auto sample=aoe2hd::ReadSelectedTelemetry(memory,world);
  const State current=stateOf(sample);
  if(lockedId==0&&sample.status==aoe2hd::TelemetryStatus::Ready&&
     sample.identity.candidateId!=0&&sample.ownership==aoe2hd::EntityOwnership::Local){
   lockedId=sample.identity.candidateId;
   std::cout<<"Locked entity candidate ID "<<lockedId<<". Keep it selected.\n";
  }
  const bool active=lockedId!=0&&current.id==lockedId&&
    current.status==aoe2hd::TelemetryStatus::Ready;
  bool markerChanged=false;
  while(_kbhit()){
   const int v=_getch();
   if(v=='q'||v=='Q'){done=true;break;}
   const char* role=aoe2hd::VillagerHudRoleKey(static_cast<char>(v));
   const char* act=aoe2hd::VillagerActionKey(static_cast<char>(v));
   if(role){hudRole=role;markerChanged=true;
    std::cout<<"HUD role observed: "<<hudRole<<" (manual)\n";}
   if(act){action=act;markerChanged=true;
    std::cout<<"Action observed: "<<action<<" (manual)\n";}
  }
  const bool stateChanged=first||differs(current,previous);
  if(stateChanged||markerChanged){
   const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now()-started).count();
   const char* event=markerChanged?"marker":(active?"observation":"selection_status");
   out<<elapsed<<','<<event<<','<<hudRole<<','<<action<<','<<lockedId<<','
      <<current.object<<','<<current.id<<','
      <<aoe2hd::EntityOwnershipName(current.owner)<<','
      <<aoe2hd::TelemetryStatusName(current.status)<<','
      <<current.typeRef<<','<<current.stable<<','<<current.readable<<',';
   if(current.readable)out<<current.a;
   out<<',';
   if(current.readable)out<<current.b;
   out<<','<<sample.hp<<'\n';out.flush();
   if(active)std::cout<<"["<<elapsed<<" ms] "<<hudRole<<"/"<<action
    <<" id="<<current.id<<" ref="<<current.typeRef
    <<" typeA="<<(current.readable?std::to_string(current.a):"unreadable")
    <<" typeB="<<(current.readable?std::to_string(current.b):"unreadable")<<'\n';
  }
  // Never automatically adopt a different villager after the initial lock.
  first=false;previous=current;
  if(!done)std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 std::cout<<"Saved "<<filename<<'\n';
 return 0;
}
