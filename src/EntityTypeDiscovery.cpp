#include "EntityTypeDiscovery.h"
#include <algorithm>
#include <set>
namespace aoe2hd {
EntityTypeSample SampleEntityType(const MemoryReader& memory, uint32_t object,
 const std::string& label,uint32_t bytes){
 EntityTypeSample s;s.object=object;s.label=label;
 bytes=std::min(bytes,0x400u)&~3u;
 for(uint32_t offset=0;offset<bytes;offset+=4){
  EntityTypeField f;f.offset=offset;
  if(object>=0x10000 && object<=UINT32_MAX-offset)
   f.readable=memory.read(object+offset,&f.raw,4);
  s.fields.push_back(f);
 }
 return s;
}
std::vector<EntityTypeComparison> CompareEntityTypes(const std::vector<EntityTypeSample>& samples){
 std::vector<EntityTypeComparison> out;
 if(samples.size()<2)return out;
 size_t limit=samples.front().fields.size();
 for(const auto& sample:samples)limit=std::min(limit,sample.fields.size());
 for(size_t i=0;i<limit;++i){
  EntityTypeComparison c;c.offset=samples.front().fields[i].offset;c.allReadable=true;
  std::set<uint32_t> values;
  for(const auto& sample:samples){
   const auto& f=sample.fields[i];
   if(!f.readable||f.offset!=c.offset){c.allReadable=false;break;}
   values.insert(f.raw);
  }
  c.distinctValues=static_cast<uint32_t>(values.size());
  c.differentiates=c.allReadable&&values.size()>1;
  out.push_back(c);
 }
 return out;
}
}
