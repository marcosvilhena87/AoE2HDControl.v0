#include "SelectedTelemetryReader.h"
#include "ProcessMemory.h"
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
namespace {
volatile std::sig_atomic_t stopped=0;
void stop(int){stopped=1;}
}
int main(int argc,char** argv){
 std::string path="aoe2hd_selected_telemetry.csv";
 if(argc==3&&std::string(argv[1])=="--csv")path=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_selected_telemetry [--csv file]\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream out(path,std::ios::trunc);
 if(!out){std::cerr<<"Cannot create CSV\n";return 2;}
 out<<"elapsed_ms,world,local_player,status,slot_15c,slot_160,object,hp_float,hp_raw,identity_readable,candidate_id_08,signature_00,candidate_type_ref_0c,candidate_owner_10,ownership\n";
 std::signal(SIGINT,stop);
 const auto start=std::chrono::steady_clock::now();
 bool first=true;aoe2hd::SelectedTelemetry previous;uint32_t lastWorld=0,lastLocal=0;
 std::cout<<"Read-only selected telemetry; Ctrl+C to stop.\n";
 while(!stopped){
  const auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
  const auto* local=aoe2hd::GetLocalPlayer(world);
  const uint32_t worldPtr=world.ready()?world.snapshot.world:0;
  const uint32_t localPtr=local?local->object:0;
  const auto sample=aoe2hd::ReadSelectedTelemetry(memory,world);
  const bool changed=first||lastWorld!=worldPtr||lastLocal!=localPtr||
   sample.status!=previous.status||sample.object!=previous.object||
   sample.slot15c!=previous.slot15c||sample.slot160!=previous.slot160||
   sample.hpRaw!=previous.hpRaw||
   sample.identity.readable!=previous.identity.readable||
   sample.identity.candidateId!=previous.identity.candidateId||
   sample.identity.candidateOwner!=previous.identity.candidateOwner||
   sample.ownership!=previous.ownership;
  if(changed){
   const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
       std::chrono::steady_clock::now()-start).count();
   out<<ms<<','<<worldPtr<<','<<localPtr<<','
    <<aoe2hd::TelemetryStatusName(sample.status)<<','
    <<sample.slot15c<<','<<sample.slot160<<','<<sample.object<<','
    <<sample.hp<<','<<sample.hpRaw<<','<<sample.identity.readable<<','
    <<sample.identity.candidateId<<','<<sample.identity.signature<<','
    <<sample.identity.candidateTypeRef<<','<<sample.identity.candidateOwner<<','
    <<aoe2hd::EntityOwnershipName(sample.ownership)<<'\n';
   out.flush();
   std::cout<<"["<<ms<<" ms] "<<aoe2hd::TelemetryStatusName(sample.status)
    <<" object=0x"<<std::hex<<sample.object<<std::dec
    <<" id="<<sample.identity.candidateId
    <<" owner="<<aoe2hd::EntityOwnershipName(sample.ownership)
    <<" hp="<<sample.hp<<'\n';
  }
  first=false;previous=sample;lastWorld=worldPtr;lastLocal=localPtr;
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 return 0;
}
