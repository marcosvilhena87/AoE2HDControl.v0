#include "EntityHealthMonitor.h"
#include "ProcessMemory.h"
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <thread>
#include <vector>
namespace {
volatile std::sig_atomic_t stopped=0;
void stop(int){stopped=1;}
bool parseAddress(const std::string& s,uint32_t& address){
 if(s.empty()||s[0]=='-'||s[0]=='+')return false;
 errno=0;char* end=nullptr;
 const unsigned long long n=std::strtoull(s.c_str(),&end,0);
 if(errno||end==s.c_str()||*end!='\0'||n<0x10000ULL||
    n>UINT32_MAX-aoe2hd::EntityHpFieldOffset)return false;
 address=static_cast<uint32_t>(n);return true;
}
std::string readPhase(const std::string& file){
 std::ifstream stream(file);std::string s;
 if(stream&&std::getline(stream,s)&&!s.empty()&&s.size()<=32)return s;
 return "unmarked";
}
}
int main(int argc,char** argv){
 std::vector<uint32_t> bases;std::set<uint32_t> seen;
 std::string csvPath="aoe2hd_entity_health.csv",phasePath="entity_health_phase.txt";
 for(int i=1;i<argc;++i){
  const std::string arg=argv[i];
  if((arg=="--base"||arg=="--hp-address")&&i+1<argc){
   uint32_t address=0;
   if(!parseAddress(argv[++i],address)){
    std::cerr<<"Invalid address for "<<arg<<"\n";return 2;
   }
   uint32_t base=address;
   if(arg=="--hp-address"){
    if(address<aoe2hd::EntityHpFieldOffset){std::cerr<<"HP address too small\n";return 2;}
    base=address-aoe2hd::EntityHpFieldOffset;
   }
   if(seen.insert(base).second)bases.push_back(base);
  }else if(arg=="--csv"&&i+1<argc)csvPath=argv[++i];
  else if(arg=="--phase-file"&&i+1<argc)phasePath=argv[++i];
  else{std::cerr<<"Usage: aoe2hd_entity_health (--base 0xADDRESS | --hp-address 0xADDRESS) ... [--csv file] [--phase-file file]\n";return 2;}
 }
 if(bases.empty()){std::cerr<<"Provide at least one --base or --hp-address\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream csv(csvPath,std::ios::trunc);
 if(!csv){std::cerr<<"Cannot open health CSV\n";return 2;}
 csv<<"elapsed_ms,phase,world,player,base,hp_address,field_offset,readable,hp_float,raw_u32,plausible,kind\n";
 csv.flush();
 std::signal(SIGINT,stop);
 std::cout<<"Read-only entity health probe: candidate HP offset +0x34; Ctrl+C to stop.\n";
 aoe2hd::EntityHealthMonitor monitor;
 uint32_t lastWorld=0,lastPlayer=0;
 std::string lastPhase;
 const auto start=std::chrono::steady_clock::now();
 while(!stopped){
  const auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
  const auto* player=aoe2hd::GetLocalPlayer(world);
  const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
     std::chrono::steady_clock::now()-start).count();
  if(!player){
   monitor.reset();lastWorld=lastPlayer=0;
   std::this_thread::sleep_for(std::chrono::milliseconds(250));continue;
  }
  if(world.snapshot.world!=lastWorld||player->object!=lastPlayer){
   monitor.reset();lastWorld=world.snapshot.world;lastPlayer=player->object;
  }
  const auto phase=readPhase(phasePath);
  const bool phaseChanged=phase!=lastPhase;
  if(phaseChanged){lastPhase=phase;std::cout<<"["<<ms<<" ms] phase="<<phase<<"\n";}
  auto events=monitor.observe(memory,bases,phaseChanged);
  for(const auto& event:events){
   csv<<ms<<','<<phase<<','<<world.snapshot.world<<','<<player->object
      <<','<<event.base<<','<<event.address<<','<<aoe2hd::EntityHpFieldOffset
      <<','<<event.readable<<','<<event.hp<<','<<event.raw
      <<','<<event.plausible<<','<<event.kind<<'\n';
   if(event.kind=="change")
    std::cout<<"["<<ms<<" ms] hp_address=0x"<<std::hex<<event.address
      <<std::dec<<" hp="<<event.hp<<"\n";
  }
  if(!events.empty())csv.flush();
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 return 0;
}
