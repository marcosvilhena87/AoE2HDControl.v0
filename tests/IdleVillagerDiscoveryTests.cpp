#include "IdleVillagerDiscovery.h"
#include <cassert>
#include <iostream>
using namespace aoe2hd;
int main(){
 std::vector<IdleObservation> s(4);
 for(size_t i=0;i<s.size();++i){
  s[i].idleButtonSelected=i>=2;
  s[i].fields.fields={{0,static_cast<uint32_t>(i>=2?1:0),true},
   {4,42,true},{8,static_cast<uint32_t>(i),true}};
 }
 auto a=CompareIdleObservations(s);
 assert(a.size()==3);
 assert(a[0].allReadable&&a[0].disjoint&&a[0].workingCount==2&&a[0].idleCount==2);
 assert(!a[1].disjoint&&a[2].disjoint); // dynamic values are only candidates
 s[3].fields.fields[0].readable=false;
 a=CompareIdleObservations(s);
 assert(!a[0].allReadable&&!a[0].disjoint);
 std::cout<<"Idle villager discovery tests passed\n";
}