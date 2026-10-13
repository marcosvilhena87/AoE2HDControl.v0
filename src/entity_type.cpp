#include "ProcessMemory.h"
#include "EntityTypeDiscovery.h"
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
namespace {
bool parse(const std::string& s,uint32_t& value){
 if(s.empty()||s[0]=='-'||s[0]=='+')return false;
 errno=0;char* end=nullptr;
 auto n=std::strtoull(s.c_str(),&end,0);
 if(errno||end==s.c_str()||*end!='\0'||n<0x10000ULL||n>UINT32_MAX-0x400u)return false;
 value=static_cast<uint32_t>(n);return true;
}
bool labelValid(const std::string& s){
 return !s.empty()&&s.size()<=40&&s.find_first_of(",\r\n")==std::string::npos;
}
}
int main(int argc,char** argv){
 struct Entry{std::string name;uint32_t base;};
 std::vector<Entry> entries;std::set<uint32_t> seen;
 std::string prefix="aoe2hd_entity_type";
 uint32_t bytes=0x100;
 for(int i=1;i<argc;++i){
  const std::string arg=argv[i];
  if(arg=="--entity"&&i+2<argc){
   std::string name=argv[++i];uint32_t base=0;
   if(!labelValid(name)||!parse(argv[++i],base)||!seen.insert(base).second){
    std::cerr<<"Invalid or duplicate --entity\n";return 2;
   }
   entries.push_back({name,base});
  }else if(arg=="--prefix"&&i+1<argc)prefix=argv[++i];
  else if(arg=="--bytes"&&i+1<argc){
   const std::string amount=argv[++i];
   char* end=nullptr;errno=0;
   const auto parsed=std::strtoull(amount.c_str(),&end,0);
   if(amount.empty()||amount[0]=='-'||amount[0]=='+'||errno||
      end==amount.c_str()||*end!='\0'||parsed<4||parsed>0x400u||parsed%4u!=0){
    std::cerr<<"Invalid --bytes; use 0x100 or 0x400\n";return 2;
   }
   bytes=static_cast<uint32_t>(parsed);
  }else{std::cerr<<"Usage: aoe2hd_entity_type --entity villager 0xBASE --entity house 0xBASE [--prefix path]\n";return 2;}
 }
 if(entries.size()<2){std::cerr<<"Provide at least two known object bases\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream raw(prefix+"_fields.csv",std::ios::trunc),cmp(prefix+"_comparison.csv",std::ios::trunc);
 if(!raw||!cmp){std::cerr<<"Cannot create CSV\n";return 2;}
 raw<<"label,object,offset,readable,raw_u32\n";
 cmp<<"offset,all_readable,distinct_values,differentiates\n";
 std::vector<aoe2hd::EntityTypeSample> samples;
 for(const auto& e:entries){
  auto s=aoe2hd::SampleEntityType(memory,e.base,e.name,bytes);
  for(const auto& f:s.fields)
   raw<<s.label<<','<<s.object<<','<<f.offset<<','<<f.readable<<','<<f.raw<<'\n';
  samples.push_back(std::move(s));
 }
 for(const auto& c:aoe2hd::CompareEntityTypes(samples))
  cmp<<c.offset<<','<<c.allReadable<<','<<c.distinctValues<<','<<c.differentiates<<'\n';
 std::cout<<"Read-only entity fields captured. Labels are experimental, not detected types.\n";
 return 0;
}
