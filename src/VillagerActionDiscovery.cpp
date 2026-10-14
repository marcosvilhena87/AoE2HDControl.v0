#include "VillagerActionDiscovery.h"
#include <algorithm>
namespace aoe2hd {
std::vector<ActionTripletField> CompareActionTriplet(
 const EntityTypeSample& a,const EntityTypeSample& b,const EntityTypeSample& c){
 std::vector<ActionTripletField> out;
 const size_t n=std::min({a.fields.size(),b.fields.size(),c.fields.size()});
 for(size_t i=0;i<n;++i){
  const auto& x=a.fields[i];const auto& y=b.fields[i];const auto& z=c.fields[i];
  ActionTripletField t;t.offset=x.offset;
  t.readable=x.readable&&y.readable&&z.readable&&x.offset==y.offset&&y.offset==z.offset;
  if(t.readable){
   t.gathering1=x.raw;t.stopped=y.raw;t.gathering2=z.raw;
   t.changed=x.raw!=y.raw;
   t.reverses=t.changed&&x.raw==z.raw;
  }
  out.push_back(t);
 }
 return out;
}
}