#include "EntityReferenceDiscovery.h"
#include <cassert>
#include <map>
struct Mock:aoe2hd::MemoryReader{
 std::map<uint32_t,uint32_t> m;
 bool read(uint32_t a,void* d,size_t n)const override{
  if(n!=4)return false;auto p=m.find(a);if(p==m.end())return false;
  *static_cast<uint32_t*>(d)=p->second;return true;
 }
};
int main(){
 Mock m;m.m[0x1000]=17;m.m[0x1004]=0x41220000;
 m.m[0x1008]=0x41220000;m.m[0x100c]=12;
 auto v=aoe2hd::FindExactReferences(m,0x1000,16,0x41220000);
 assert(v.size()==2&&v[0].address==0x1004&&v[1].address==0x1008);
 assert(aoe2hd::FindExactReferences(m,0x1000,16,0x41220000,1).size()==1);
 assert(aoe2hd::FindExactReferences(m,0x1000,3,0x41220000).empty());
 assert(aoe2hd::FindExactReferences(m,0xfffffffc,16,0x41220000).empty());
}