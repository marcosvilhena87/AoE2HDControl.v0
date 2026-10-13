#include "EntityTypeResolver.h"
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
 if(errno||end==s.c_str()||*end!='\0'||n<0x10000ULL||n>UINT32_MAX-0x400u||n%4)return false;
 out=static_cast<uint32_t>(n);return true;
}
}
int main(int argc,char** argv){
 struct Input{std::string label;uint32_t base;};
 std::vector<Input> inputs;std::set<uint32_t> seen;
 std::string prefix="aoe2hd_type_reference";
 for(int i=1;i<argc;++i){
  const std::string arg=argv[i];
  if(arg=="--entity"&&i+2<argc){
   const std::string label=argv[++i];uint32_t base=0;
   if(label.empty()||label.size()>40||label.find_first_of(",\r\n")!=std::string::npos||
      !parse(argv[++i],base)||!seen.insert(base).second){
    std::cerr<<"Invalid or duplicate entity\n";return 2;
   }
   inputs.push_back({label,base});
  }else if(arg=="--prefix"&&i+1<argc)prefix=argv[++i];
  else{std::cerr<<"Usage: aoe2hd_type_reference --entity label 0xBASE --entity label 0xBASE [--prefix name]\n";return 2;}
 }
 if(inputs.size()<2){std::cerr<<"Provide at least two known entities\n";return 2;}
 aoe2hd::ProcessMemory memory;
 if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
 std::ofstream fields(prefix+"_fields.csv",std::ios::trunc),
   summary(prefix+"_summary.csv",std::ios::trunc),
   comparison(prefix+"_comparison.csv",std::ios::trunc);
 if(!fields||!summary||!comparison){std::cerr<<"Cannot open output CSV files\n";return 2;}
 fields<<"label,object,type_reference,stable,offset,readable,raw_u32\n";
 summary<<"label,object,identity_readable,candidate_id,type_reference,stable,readable_fields\n";
 comparison<<"offset,all_readable,distinct_values,differentiates\n";
 std::vector<aoe2hd::EntityTypeSample> samples;
 for(const auto& entry:inputs){
  const auto s=aoe2hd::SampleTypeReference(memory,entry.base,entry.label);
  uint32_t readable=0;
  for(const auto& f:s.definition.fields){
   if(f.readable)++readable;
   fields<<entry.label<<','<<entry.base<<','<<s.before<<','<<s.stable
    <<','<<f.offset<<','<<f.readable<<','<<f.raw<<'\n';
  }
  summary<<entry.label<<','<<entry.base<<','<<s.identity.readable<<','
    <<s.identity.candidateId<<','<<s.before<<','<<s.stable<<','<<readable<<'\n';
  if(s.stable)samples.push_back(s.definition);
 }
 if(samples.size()>=2){
  for(const auto& row:aoe2hd::CompareEntityTypes(samples))
   comparison<<row.offset<<','<<row.allReadable<<','<<row.distinctValues
    <<','<<row.differentiates<<'\n';
 }
 std::cout<<"Type-reference candidates captured; no type IDs are confirmed.\n";
 return 0;
}
