#include "GatheringActivityMonitor.h"
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
 out<<"elapsed_ms,entity_id,object,definition_ref,load,window_ms,window_gain,complete_window,activity_evidence\n";
 aoe2hd::GatheringActivityMonitor monitor(2000);
 uint32_t locked=0,lockedObject=0;
 auto started=std::chrono::steady_clock::now();
 aoe2hd::GatheringActivity last=aoe2hd::GatheringActivity::Unknown;
 std::cout<<"Read-only GatheringActivityMonitor v2; Ctrl+C to end.\n"
  <<"Select ONE local villager. Window 2000 ms; full load threshold 10 is experimental.\n"
  <<"No recent progress does NOT mean idle.\n";
 while(mem.isAlive()){
  auto s=aoe2hd::ReadSelectedVillagerCarry(mem,aoe2hd::ReadWorld(mem,mem.moduleBase()));
  uint64_t elapsed=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
   std::chrono::steady_clock::now()-started).count());
  if(s.valid&&!locked){locked=s.id;lockedObject=s.object;
   std::cout<<"Locked id="<<locked<<" object="<<lockedObject<<'\n';}
  if(s.valid&&(s.id!=locked||s.object!=lockedObject))s.valid=false;
  auto v=monitor.Observe(elapsed,s);
  out<<elapsed<<',';
  if(s.valid)out<<s.id<<','<<s.object<<','<<s.definition<<','
    <<std::setprecision(9)<<s.load;
  else out<<",,,";
  out<<','<<v.windowMs<<','<<v.windowGain<<','<<v.completeWindow<<','
    <<aoe2hd::GatheringActivityName(v.state)<<'\n';
  out.flush();
  if(v.state!=last){
   std::cout<<"["<<elapsed<<" ms] "<<aoe2hd::GatheringActivityName(v.state);
   if(s.valid)std::cout<<" load="<<s.load<<" gain="<<v.windowGain;
   std::cout<<'\n';last=v.state;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 return 0;
}
