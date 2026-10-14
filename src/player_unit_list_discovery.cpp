#include "UnitCollectionDiscovery.h"
#include "SelectedTelemetryReader.h"
#include "ProcessMemory.h"
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <thread>
namespace {
volatile std::sig_atomic_t stopRequested=0;
void onStop(int){stopRequested=1;}
}
int main(int argc,char** argv){
 std::string path="aoe2hd_player_unit_lists.csv";
 if(argc==3&&std::string(argv[1])=="--csv")path=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_player_unit_list_discovery [--csv file]\n";return 2;}
 aoe2hd::ProcessMemory mem;
 if(!mem.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream out(path,std::ios::trunc);
 if(!out){std::cerr<<"Cannot create CSV\n";return 2;}
 out<<"elapsed_ms,world,player,source,offset,stride,entries,sampled,local_matches,other_matches,unreadable,selected_matches,ids_sample\n";
 std::ofstream detail("aoe2hd_player_unit_entry_evidence.csv",std::ios::trunc);
 if(!detail){std::cerr<<"Cannot create entry evidence CSV\n";return 2;}
 detail<<"elapsed_ms,source,offset,stride,index,word,value,target,entity_id,owner,kind,selected_match,local_owner\n";
 std::signal(SIGINT,onStop);
 std::cout<<"Read-only player unit list discovery v2; Ctrl+C to stop.\n"
 <<"Select a known unit occasionally to anchor candidate lists; selection is optional.\n"
 <<"Rows are hypotheses, not confirmed unit enumerations.\n";
 auto started=std::chrono::steady_clock::now();
 while(!stopRequested&&mem.isAlive()){
  auto world=aoe2hd::ReadWorld(mem,mem.moduleBase());
  auto local=aoe2hd::GetLocalPlayer(world);
  if(local){
   auto selected=aoe2hd::ReadSelectedTelemetry(mem,world);
   uint32_t anchor=selected.status==aoe2hd::TelemetryStatus::Ready?selected.object:0;
   auto report=aoe2hd::InspectUnitCollections(mem,world,anchor,0x800,512,64);
   const auto& result=report.observations;
   auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
     std::chrono::steady_clock::now()-started).count();
   for(const auto& entry:result){
    for(const auto& ev:entry.entries){
     detail<<ms<<','<<entry.summary.vector.source<<','<<entry.summary.vector.offset<<','
      <<entry.summary.vector.stride<<','<<ev.index<<','<<ev.word<<','
      <<ev.value<<','<<ev.target<<','<<ev.entityId<<','
      <<ev.owner<<','<<ev.kind<<','<<ev.selectedMatch<<','<<ev.localOwner<<'\n';
    }
    const auto& v=entry.summary.vector;
    out<<ms<<','<<world.snapshot.world<<','<<local->object<<','
     <<v.source<<','<<v.offset<<','<<v.stride<<','<<v.count<<','
     <<entry.summary.sampled<<','<<entry.summary.localMatches<<','<<entry.summary.otherMatches<<','
     <<entry.summary.unreadable<<','<<entry.summary.selectedMatches<<',';
    for(size_t j=0;j<entry.summary.sampledIds.size();++j){
     if(j)out<<';';
     out<<entry.summary.sampledIds[j];
    }
    out<<'\n';
   }
   out.flush();detail.flush();
   if(!result.empty()){
    const auto& best=result.front().summary;
    std::cout<<"["<<ms<<" ms] candidates="<<result.size()<<" best="
     <<best.vector.source<<"+0x"<<std::hex<<best.vector.offset<<std::dec
     <<" count="<<best.vector.count<<" local="<<best.localMatches
     <<" selected="<<best.selectedMatches<<'\n';
   }else std::cout<<"["<<ms<<" ms] no vector candidates in scanned roots\n";
  }
  for(int n=0;n<4&&!stopRequested;++n)
   std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 return 0;
}