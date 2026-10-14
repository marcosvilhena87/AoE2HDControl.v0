#include "EntityReferenceDiscovery.h"
#include "SelectedTelemetryReader.h"
#include "ProcessMemory.h"
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <thread>
namespace {
volatile std::sig_atomic_t stopped=0;
void signalStop(int){stopped=1;}
}
int main(int argc,char** argv){
 std::string csv="aoe2hd_entity_references.csv";
 if(argc==3&&std::string(argv[1])=="--csv")csv=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_entity_reference_discovery [--csv path]\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream out(csv,std::ios::trunc);
 if(!out){std::cerr<<"Cannot create CSV\n";return 2;}
 out<<"elapsed_ms,entity_id,entity_object,source,region_base,reference_address,relative_offset\n";
 std::signal(SIGINT,signalStop);
 auto start=std::chrono::steady_clock::now();
 std::set<uint64_t> logged;
 uint32_t lastAnchor=0;
 std::cout<<"EntityReferenceDiscovery v1 read-only; select one unit at a time. Ctrl+C to stop.\n"
 <<"Scanning bounded 16 KiB windows at World and LocalPlayer roots.\n";
 while(!stopped&&memory.isAlive()){
  auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
  const auto* player=aoe2hd::GetLocalPlayer(world);
  if(player){
   auto selection=aoe2hd::ReadSelectedTelemetry(memory,world);
   if(selection.status==aoe2hd::TelemetryStatus::Ready&&
      selection.object&&selection.identity.candidateId){
    const uint32_t anchor=selection.object;
    if(anchor!=lastAnchor){logged.clear();lastAnchor=anchor;
     std::cout<<"Anchor id="<<selection.identity.candidateId<<" object="<<anchor<<'\n';}
    struct Region {uint32_t base;const char* source;};
    const Region regions[]={{world.snapshot.world,"World"},{player->object,"LocalPlayer"}};
    for(const auto& region:regions){
     if(region.base>UINT32_MAX-0x4000u)continue;
     auto hits=aoe2hd::FindExactReferences(memory,region.base,0x4000,anchor,512);
     auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
       std::chrono::steady_clock::now()-start).count();
     for(const auto& hit:hits){
      uint64_t key=(static_cast<uint64_t>(region.base)<<32)|hit.address;
      if(!logged.insert(key).second)continue;
      out<<ms<<','<<selection.identity.candidateId<<','<<anchor<<','
       <<region.source<<','<<region.base<<','<<hit.address<<','
       <<hit.address-region.base<<'\n';
      std::cout<<"Reference "<<region.source<<"+0x"<<std::hex
       <<(hit.address-region.base)<<std::dec<<" -> "<<anchor<<'\n';
     }
    }
    out.flush();
   }
  }
  for(int i=0;i<4&&!stopped;++i)std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 return 0;
}
