#include "ProcessMemory.h"
#include "ConstructionProgressMonitor.h"
#include <chrono>
#include <cerrno>
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
std::string phaseFrom(const std::string& path){
 std::ifstream input(path);std::string s;
 if(input&&std::getline(input,s)&&!s.empty()&&s.size()<=32)return s;
 return "unmarked";
}
bool addressFrom(const std::string& s,uint32_t& out){
 if(s.empty()||s[0]=='-'||s[0]=='+')return false;
 errno=0;char* end=nullptr;
 const auto v=std::strtoull(s.c_str(),&end,0);
 if(errno||end==s.c_str()||*end!='\0'||v<0x10000ULL||
     v>static_cast<unsigned long long>(UINT32_MAX-aoe2hd::ConstructionProgressFieldOffset))return false;
 out=static_cast<uint32_t>(v);return true;
}
}
int main(int argc,char** argv){
 std::vector<uint32_t> bases;std::set<uint32_t> seen;
 std::string csvPath="aoe2hd_construction_progress.csv",phasePath="construction_phase.txt";
 for(int i=1;i<argc;++i){
  const std::string arg=argv[i];
  if(arg=="--base"&&i+1<argc){
   uint32_t base=0;
   if(!addressFrom(argv[++i],base)){std::cerr<<"Invalid --base\n";return 2;}
   if(seen.insert(base).second)bases.push_back(base);
  }else if(arg=="--csv"&&i+1<argc)csvPath=argv[++i];
  else if(arg=="--phase-file"&&i+1<argc)phasePath=argv[++i];
  else {std::cerr<<"Usage: aoe2hd_construction_progress --base 0xADDRESS [--base 0xADDRESS] [--csv file] [--phase-file file]\n";return 2;}
 }
 if(bases.empty()){std::cerr<<"At least one --base required\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream csv(csvPath,std::ios::trunc);
 if(!csv){std::cerr<<"Cannot open CSV\n";return 2;}
 csv<<"elapsed_ms,phase,world,player,base,field_offset,address,readable,float_value,raw_u32,in_range,kind\n";
 csv.flush();
 std::signal(SIGINT,stop);
 std::cout<<"Read-only construction monitor; offset=0xC1C; Ctrl+C to stop.\n";
 aoe2hd::ConstructionProgressMonitor monitor;
 uint32_t lastWorld=0,lastPlayer=0;
 std::string lastPhase;
 const auto start=std::chrono::steady_clock::now();
 while(!stopped){
  const auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
  const auto* player=aoe2hd::GetLocalPlayer(world);
  const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now()-start).count();
  if(!player){monitor.reset();lastWorld=lastPlayer=0;
   std::this_thread::sleep_for(std::chrono::milliseconds(250));continue;}
  if(world.snapshot.world!=lastWorld||player->object!=lastPlayer){
   monitor.reset();lastWorld=world.snapshot.world;lastPlayer=player->object;
  }
  const auto phase=phaseFrom(phasePath);
  const bool phaseChanged=phase!=lastPhase;
  if(phaseChanged){lastPhase=phase;std::cout<<"["<<ms<<" ms] phase="<<phase<<"\n";}
  auto events=monitor.observe(memory,bases,phaseChanged);
  for(const auto& e:events){
   csv<<ms<<','<<phase<<','<<world.snapshot.world<<','<<player->object
      <<','<<e.base<<','<<aoe2hd::ConstructionProgressFieldOffset<<','
      <<e.address<<','<<e.readable<<','<<e.value<<','<<e.raw
      <<','<<e.inRange<<','<<e.kind<<'\n';
   if(e.kind=="change")std::cout<<"["<<ms<<" ms] base=0x"<<std::hex<<e.base
       <<std::dec<<" float="<<e.value<<"\n";
  }
  if(!events.empty())csv.flush();
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
 }
 return 0;
}
