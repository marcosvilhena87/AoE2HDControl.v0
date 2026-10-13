#include "EntityIdentityReader.h"
#include "ProcessMemory.h"
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
namespace {
bool parse(const std::string& s,uint32_t& out){
 if(s.empty()||s[0]=='-'||s[0]=='+')return false;
 errno=0;char* end=nullptr;
 const auto n=std::strtoull(s.c_str(),&end,0);
 if(errno||end==s.c_str()||*end!='\0'||n<0x10000ULL||n>UINT32_MAX-0x10u||n%4)return false;
 out=static_cast<uint32_t>(n);return true;
}
}
int main(int argc,char** argv){
 struct Input {std::string label;uint32_t object;};
 std::vector<Input> inputs;std::set<uint32_t> seen;
 std::string path="aoe2hd_entity_identity.csv";
 for(int i=1;i<argc;++i){
  const std::string arg=argv[i];
  if(arg=="--entity"&&i+2<argc){
   const std::string label=argv[++i];uint32_t address=0;
   if(label.empty()||label.size()>40||label.find_first_of(",\r\n")!=std::string::npos||
      !parse(argv[++i],address)||!seen.insert(address).second){
    std::cerr<<"Invalid or duplicate entity\n";return 2;
   }
   inputs.push_back({label,address});
  }else if(arg=="--csv"&&i+1<argc)path=argv[++i];
  else{std::cerr<<"Usage: aoe2hd_entity_identity --entity label 0xBASE [--entity label 0xBASE] [--csv path]\n";return 2;}
 }
 if(inputs.empty()){std::cerr<<"At least one --entity required\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream out(path,std::ios::trunc);
 if(!out){std::cerr<<"Cannot open CSV\n";return 2;}
 const auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
 out<<"label,object,readable,signature_00,candidate_id_08,candidate_type_ref_0c,candidate_owner_10,relation_to_first,ownership,world_ready,local_player\n";
 std::vector<aoe2hd::EntityIdentity> samples;
 for(const auto& input:inputs)samples.push_back(aoe2hd::ReadEntityIdentity(memory,input.object));
 for(size_t i=0;i<inputs.size();++i){
  const auto& s=samples[i];
  std::string relation="unknown";
  if(i!=0){
   const auto relationCode=aoe2hd::CompareCandidateOwners(samples[0],s);
   if(relationCode==aoe2hd::OwnerRelation::SameCandidate)relation="same_candidate";
   if(relationCode==aoe2hd::OwnerRelation::DifferentCandidate)relation="different_candidate";
  }
  out<<inputs[i].label<<','<<s.object<<','<<s.readable<<','<<s.signature
    <<','<<s.candidateId<<','<<s.candidateTypeRef<<','<<s.candidateOwner
    <<','<<relation<<','<<aoe2hd::EntityOwnershipName(aoe2hd::ClassifyEntityOwnership(s,world))
    <<','<<world.ready()<<','<<(aoe2hd::GetLocalPlayer(world)?aoe2hd::GetLocalPlayer(world)->object:0)<<'\n';
 }
 std::cout<<"Candidate identity fields saved: "<<path
  <<". Values are not yet confirmed IDs or owners.\n";
 return 0;
}
