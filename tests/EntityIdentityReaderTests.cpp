#include "EntityIdentityReader.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
using namespace aoe2hd;
struct Fake final:MemoryReader {
 std::map<uint32_t,uint32_t> words;
 bool read(uint32_t address,void* dst,size_t n) const override {
  if(n!=20)return false;
  uint32_t v[5]={};
  for(uint32_t i=0;i<5;++i){
   auto it=words.find(address+4*i);if(it==words.end())return false;
   v[i]=it->second;
  }
  std::memcpy(dst,v,20);return true;
 }
 void assign(uint32_t p,uint32_t owner,uint32_t id){
  words[p]=0x123456;words[p+4]=7;words[p+8]=id;
  words[p+12]=0x456789;words[p+16]=owner;
 }
};
int main(){
 Fake f;f.assign(0x10000,0x20000,19);f.assign(0x11000,0x20000,21);
 f.assign(0x12000,0x30000,23);
 auto a=ReadEntityIdentity(f,0x10000),b=ReadEntityIdentity(f,0x11000),c=ReadEntityIdentity(f,0x12000);
 assert(a.readable&&a.candidateId==19&&a.candidateOwner==0x20000&&a.candidateTypeRef==0x456789);
 assert(CompareCandidateOwners(a,b)==OwnerRelation::SameCandidate);
 assert(CompareCandidateOwners(a,c)==OwnerRelation::DifferentCandidate);
 assert(CompareCandidateOwners(a,ReadEntityIdentity(f,0x13000))==OwnerRelation::Unknown);
 assert(!ReadEntityIdentity(f,0x10).readable);
 f.words.erase(0x12000+12);
 assert(!ReadEntityIdentity(f,0x12000).readable);
 std::cout<<"Entity identity reader tests passed\n";
}
