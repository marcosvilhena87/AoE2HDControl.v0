#include "ProcessMemory.h"
#include "ProcessReferenceScanner.h"
#include "SelectedTelemetryReader.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <csignal>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>
namespace {
volatile std::sig_atomic_t stopped=0;
void stop(int){stopped=1;}
}
int main(int argc,char** argv){
 std::string csv="aoe2hd_process_references.csv";
 if(argc==3&&std::string(argv[1])=="--csv")csv=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_process_reference_discovery [--csv path]\n";return 2;}
 aoe2hd::ProcessMemory mem;
 if(!mem.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream out(csv,std::ios::trunc);
 if(!out){std::cerr<<"Cannot create CSV\n";return 2;}
 out<<"scan_id,entity_id,entity_object,reference_address,region_base,region_kind,classification\n";
 std::ofstream clusters("aoe2hd_reference_clusters.csv",std::ios::trunc);
 if(!clusters){std::cerr<<"Cannot create clusters CSV\n";return 2;}
 clusters<<"scan_id,group_base,distinct_ids,entity_ids\n";
 std::signal(SIGINT,stop);
 std::map<uint32_t,uint32_t> observed;
 uint32_t previousObject=0,scanId=0;
 std::cout<<"ProcessReferenceDiscovery v2 (read-only, max 256 MiB per scan).\n"
  <<"Select different individual units; each NEW unit triggers a multi-anchor scan. Ctrl+C to exit.\n";
 while(!stopped&&mem.isAlive()){
  auto world=aoe2hd::ReadWorld(mem,mem.moduleBase());
  auto selected=aoe2hd::ReadSelectedTelemetry(mem,world);
  if(world.ready()&&selected.status==aoe2hd::TelemetryStatus::Ready&&
   selected.object&&selected.identity.candidateId){
   if(selected.object!=previousObject){
    previousObject=selected.object;
    observed[selected.object]=selected.identity.candidateId;
    std::vector<aoe2hd::ScanAnchor> anchors;
    for(const auto& pair:observed)anchors.push_back({pair.second,pair.first});
    ++scanId;
    std::cout<<"Scan "<<scanId<<": known entities="<<anchors.size()<<'\n';
    const auto* local=aoe2hd::GetLocalPlayer(world);
    const uint32_t selA=local->object+0x15cu,selB=local->object+0x160u;
    uint64_t hitCount=0,bytesRead=0,failedChunks=0;
    std::map<uint32_t,std::set<uint32_t>> nearby;
    auto regions=mem.readableRegions();
    std::vector<uint8_t> buffer(65536);
    for(const auto& region:regions){
     if(stopped)break;
     for(uint64_t offset=0;offset<region.bytes&&!stopped;){
      uint32_t n=static_cast<uint32_t>((std::min)(uint64_t(buffer.size()),
       static_cast<uint64_t>(region.bytes)-offset));
      uint32_t address=region.base+static_cast<uint32_t>(offset);
      if(!mem.read(address,buffer.data(),n)){
       // Fallback to page-size blocks when a large region changes during capture.
       n=(std::min)(n,4096u);
       if(!mem.read(address,buffer.data(),n)){++failedChunks;offset+=n;continue;}
      }
      bytesRead+=n;
      for(const auto& hit:aoe2hd::ScanReferenceBlock(address,buffer.data(),n,anchors)){
       const bool selectionSlot=hit.address==selA||hit.address==selB;
       const char* kind=selectionSlot?"selection_slot":"other_reference";
       out<<scanId<<','<<hit.id<<','<<hit.object<<','<<hit.address<<','
        <<region.base<<','<<region.kind<<','<<kind<<'\n';
       if(!selectionSlot)nearby[hit.address/256].insert(hit.id);
       ++hitCount;
      }
      offset+=n;
     }
    }
    out.flush();
    unsigned groups=0;
    for(const auto& group:nearby){
     if(group.second.size()<2)continue;
     ++groups;
     clusters<<scanId<<','<<static_cast<uint64_t>(group.first)*256<<','
      <<group.second.size()<<',';
     bool first=true;
     for(uint32_t id:group.second){
      if(!first)clusters<<';';
      clusters<<id;first=false;
     }
     clusters<<'\n';
    }
    clusters.flush();
    std::cout<<"Scan "<<scanId<<" complete: read "<<bytesRead/1048576
     <<" MiB, hits="<<hitCount<<", shared 256-byte groups="<<groups
     <<", failed chunks="<<failedChunks<<'\n';
   }
  }else previousObject=0;
  for(int i=0;i<4&&!stopped;++i)
   std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 return 0;
}
