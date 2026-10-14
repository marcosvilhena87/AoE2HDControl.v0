#include "GatheringProgressDetector.h"
#include "ProcessMemory.h"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
int main(int argc,char** argv){
 std::string csv="aoe2hd_gathering_progress.csv";
 if(argc==3&&std::string(argv[1])=="--csv")csv=argv[2];
 else if(argc!=1){std::cerr<<"Usage: aoe2hd_gathering_progress [--csv path]\n";return 2;}
 aoe2hd::ProcessMemory mem;
 if(!mem.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream out(csv,std::ios::trunc);
 if(!out){std::cerr<<"Cannot write CSV\n";return 2;}
 out<<"elapsed_ms,object,entity_id,definition_ref,load_raw,load_float,previous_load,delta,event\n";
 auto started=std::chrono::steady_clock::now();
 aoe2hd::CarryReading previous;
 uint32_t lockedId=0,lockedObject=0;
 std::cout<<"Read-only candidate Entity+0x48 load monitor. Ctrl+C to end.\n"
          <<"Select one LOCAL villager; the first valid one is locked.\n"
          <<"Load increase proves progress during interval, not present idle/non-idle state.\n";
 while(mem.isAlive()){
  auto s=aoe2hd::ReadSelectedVillagerCarry(mem,aoe2hd::ReadWorld(mem,mem.moduleBase()));
  const auto time=std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now()-started).count();
  if(s.valid&&!lockedId){
   lockedId=s.id;lockedObject=s.object;
   std::cout<<"Locked id="<<lockedId<<" object="<<lockedObject<<'\n';
  }
  if(s.valid&&(s.id!=lockedId||s.object!=lockedObject))s.valid=false;
  const auto event=aoe2hd::CompareGatheringProgress(previous,s);
  if(s.valid){
   float delta=0;
   const bool comparable=previous.valid&&previous.id==s.id&&
      previous.object==s.object&&previous.definition==s.definition;
   if(comparable)delta=s.load-previous.load;
   out<<time<<','<<s.object<<','<<s.id<<','<<s.definition<<','
      <<s.raw<<','<<std::setprecision(9)<<s.load<<',';
   if(comparable)out<<previous.load;
   out<<',';
   if(comparable)out<<delta;
   out<<','<<aoe2hd::GatheringProgressName(event)<<'\n';
   out.flush();
   if(event!=aoe2hd::GatheringProgress::Unchanged)
    std::cout<<"["<<time<<" ms] load="<<s.load<<" "
             <<aoe2hd::GatheringProgressName(event)<<'\n';
  }else if(previous.valid){
   out<<time<<",,,,,,,unreadable_or_deselected\n";out.flush();
  }
  // Definition changes reset comparisons so role changes cannot be mistaken for deposits.
  previous=s;
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 std::cout<<"Saved "<<csv<<'\n';
 return 0;
}
