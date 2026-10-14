#include "VillagerActionDiscovery.h"
#include <cassert>
#include <iostream>
using namespace aoe2hd;
int main(){
 EntityTypeSample a,b,c;
 for(auto* s:{&a,&b,&c})for(uint32_t off=0;off<16;off+=4)
  s->fields.push_back({off,100+off,true});
 b.fields[1].raw=999;
 b.fields[2].raw=888;
 c.fields[2].raw=777;
 c.fields[3].readable=false;
 auto v=CompareActionTriplet(a,b,c);
 assert(v.size()==4);
 assert(v[1].reverses&&v[1].changed&&v[1].gathering1==104);
 assert(v[2].changed&&!v[2].reverses);
 assert(!v[3].readable&&!v[3].reverses);
 std::cout<<"Villager action discovery tests passed\n";
}