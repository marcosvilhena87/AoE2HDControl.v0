#include "CrossPlayerTypeExperiment.h"
#include "ProcessMemory.h"
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
namespace {
bool parse(const std::string& s,uint32_t& out){
 if(s.empty()||s[0]=='-'||s[0]=='+')return false;
 errno=0;char* end=nullptr;auto v=std::strtoull(s.c_str(),&end,0);
 if(errno||end==s.c_str()||*end!='\0'||v<0x10000ULL||v>UINT32_MAX-0x400u||(v&3u))return false;
 out=static_cast<uint32_t>(v);return true;
}
bool label(const std::string& s){return !s.empty()&&s.size()<=40&&s.find_first_of(",\r\n")==std::string::npos;}
}
int main(int argc,char** argv){
 struct Input{std::string kind,name;uint32_t base;};
 std::vector<Input> inputs;std::string prefix="aoe2hd_cross_player_type";
 for(int i=1;i<argc;++i){
  std::string arg=argv[i];
  if(arg=="--entity"&&i+3<argc){
   std::string kind=argv[++i],name=argv[++i];uint32_t base=0;
   if(!label(kind)||!label(name)||!parse(argv[++i],base)){std::cerr<<"Invalid entity\n";return 2;}
   inputs.push_back({kind,name,base});
  }else if(arg=="--prefix"&&i+1<argc)prefix=argv[++i];
  else{std::cerr<<"Usage: aoe2hd_cross_player_type --entity KIND LABEL 0xBASE [--entity ...] [--prefix path]\n";return 2;}
 }
 if(inputs.size()<2){std::cerr<<"Provide at least two entities\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream raw(prefix+"_samples.csv",std::ios::trunc),pairs(prefix+"_pairs.csv",std::ios::trunc);
 if(!raw||!pairs){std::cerr<<"Cannot open CSV outputs\n";return 2;}
 raw<<"kind,label,object,identity_readable,owner_class,type_ref,stable,candidate_a_readable,candidate_a_08,candidate_b_readable,candidate_b_0c\n";
 pairs<<"kind,label_a,label_b,owner_a,owner_b,between_known_players,candidate_a_agreement,candidate_b_agreement\n";
 const auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
 std::vector<aoe2hd::CrossPlayerTypeRecord> samples;
 for(const auto& i:inputs){
  auto s=aoe2hd::CaptureCrossPlayerType(memory,world,i.name,i.kind,i.base);
  raw<<s.kind<<','<<s.label<<','<<s.object<<','<<s.sample.identity.readable<<','
   <<aoe2hd::EntityOwnershipName(s.ownership)<<','<<s.sample.before<<','
   <<s.sample.stable<<','<<s.candidateAReadable<<','<<s.candidateA
   <<','<<s.candidateBReadable<<','<<s.candidateB<<'\n';
  samples.push_back(s);
 }
 for(size_t a=0;a<samples.size();++a)for(size_t b=a+1;b<samples.size();++b){
  const auto& x=samples[a];const auto& y=samples[b];
  if(x.kind!=y.kind)continue;
  const bool across=(x.ownership==aoe2hd::EntityOwnership::Local&&y.ownership==aoe2hd::EntityOwnership::OtherPlayer)||
      (y.ownership==aoe2hd::EntityOwnership::Local&&x.ownership==aoe2hd::EntityOwnership::OtherPlayer);
  pairs<<x.kind<<','<<x.label<<','<<y.label<<','
   <<aoe2hd::EntityOwnershipName(x.ownership)<<','<<aoe2hd::EntityOwnershipName(y.ownership)<<','
   <<across<<','<<aoe2hd::PairVerdictName(aoe2hd::CompareCrossPlayerType(x,y,false))
   <<','<<aoe2hd::PairVerdictName(aoe2hd::CompareCrossPlayerType(x,y,true))<<'\n';
 }
 std::cout<<"Cross-player type experiment captured. Labels are operator-supplied, IDs remain candidates.\n";
 return 0;
}
