#include "EntityTypeResolver.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake:MemoryReader {
 std::map<uint32_t,uint32_t> words;
 bool read(uint32_t a,void* dst,size_t n)const override{
  if(n==4){auto it=words.find(a);if(it==words.end())return false;
   std::memcpy(dst,&it->second,4);return true;}
  if(n==20){uint32_t v[5];for(int i=0;i<5;++i){
   auto it=words.find(a+i*4);if(it==words.end())return false;v[i]=it->second;}
   std::memcpy(dst,v,20);return true;}
  return false;
 }
};
int main(){
 Fake f;const uint32_t object=0x20000,definition=0x50000;
 for(int i=0;i<5;++i)f.words[object+4*i]=0;
 f.words[object+12]=definition;f.words[definition]=123;
 auto s=SampleTypeReference(f,object,"villager",16);
 assert(s.identity.readable&&s.stable&&s.before==definition);
 assert(s.definition.fields.size()==4&&s.definition.fields[0].readable);
 assert(s.definition.fields[0].raw==123&&!s.definition.fields[1].readable);
 f.words[object+12]=0;
 assert(!SampleTypeReference(f,object,"villager").stable);
 f.words.erase(object);
 assert(!SampleTypeReference(f,object,"villager").identity.readable);
 std::cout<<"Entity type resolver tests passed\n";
}
