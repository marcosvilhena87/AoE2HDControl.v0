#include "GatheringActivityMonitor.h"
#include "GatheringProductivity.h"
#include "VillagerSelectionGuard.h"
#include "SelectedTelemetryReader.h"
#include "ProcessMemory.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
int main(int argc,char** argv){
 std::string csv="aoe2hd_gathering_activity.csv";
 if(argc==3&&std::string(argv[1])=="--csv")csv=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_gathering_activity [--csv file]\n";return 2;}
 aoe2hd::ProcessMemory mem;
 if(!mem.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream out(csv,std::ios::trunc);
 if(!out){std::cerr<<"Cannot write CSV\n";return 2;}
 out<<"elapsed_ms,selection_status,lock_status,entity_id,object,definition_ref,type_a,load,window_ms,window_gain,complete_window,activity_evidence,observed_ms,progress_ms,no_progress_ms,current_no_progress_ms,longest_no_progress_ms,collected_units,active_rate_per_s,progress_fraction\n";
 aoe2hd::GatheringActivityMonitor monitor(2000);
 aoe2hd::GatheringProductivityTracker productivity;
 uint32_t locked=0,lockedObject=0;
 auto started=std::chrono::steady_clock::now();
 std::string lastStatus;
 std::cout<<"GatheringActivityMonitor v2.2 (read-only). Ctrl+C to end.\n"
 <<"Select ONE local villager. Unsupported candidate type IDs are rejected before lock.\n"
 <<"After lock, losing/changing selection pauses tracking; restart to choose another villager.\n"
 <<"No recent progress does NOT mean idle.\n";
 while(mem.isAlive()){
  const auto world=aoe2hd::ReadWorld(mem,mem.moduleBase());
  const auto selected=aoe2hd::ReadSelectedTelemetry(mem,world);
  const auto guard=aoe2hd::CheckVillagerCandidate(selected);
  auto s=aoe2hd::CarryReading{};
  std::string status=aoe2hd::VillagerSelectionStatusName(guard);
  if(guard==aoe2hd::VillagerSelectionStatus::AcceptedCandidate){
   if(locked&&(selected.identity.candidateId!=locked||selected.object!=lockedObject))
    status="different_entity_selected";
   else {
    s=aoe2hd::ReadSelectedVillagerCarry(mem,world);
    if(!s.valid)status="carry_read_failed_or_transition";
    else if(!locked){
     locked=s.id;lockedObject=s.object;
     std::cout<<"Locked villager candidate id="<<locked<<" object="<<lockedObject
      <<" typeA="<<selected.typeCandidateA<<'\n';
    }
   }
  }
  const uint64_t elapsed=static_cast<uint64_t>(
   std::chrono::duration_cast<std::chrono::milliseconds>(
   std::chrono::steady_clock::now()-started).count());
  const std::string lock=locked?(s.valid?"tracking":"locked_waiting_for_original"):"unlocked";
  const auto activity=monitor.Observe(elapsed,s);
  const auto metrics=productivity.Observe(elapsed,s);
  out<<elapsed<<','<<status<<','<<lock<<',';
  if(selected.status==aoe2hd::TelemetryStatus::Ready)
   out<<selected.identity.candidateId<<','<<selected.object<<','
      <<selected.identity.candidateTypeRef<<',';
  else out<<",,,";
  if(selected.typeCandidatesReadable)out<<selected.typeCandidateA;
  out<<',';
  if(s.valid)out<<std::setprecision(9)<<s.load;
  out<<','<<activity.windowMs<<','<<activity.windowGain<<','
   <<activity.completeWindow<<','<<aoe2hd::GatheringActivityName(activity.state)
   <<','<<metrics.observedMs<<','<<metrics.progressMs<<','<<metrics.noProgressMs
   <<','<<metrics.currentNoProgressMs<<','<<metrics.longestNoProgressMs
   <<','<<metrics.collected<<','<<metrics.ratePerActiveSecond
   <<','<<metrics.progressFraction<<'\n';
  out.flush();
  std::string combined=status+"/"+lock+"/"+aoe2hd::GatheringActivityName(activity.state);
  if(combined!=lastStatus){
   std::cout<<"["<<elapsed<<" ms] "<<combined;
   if(s.valid)std::cout<<" load="<<s.load<<" gain="<<activity.windowGain;
   std::cout<<'\n';lastStatus=combined;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 return 0;
}
