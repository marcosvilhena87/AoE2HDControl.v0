#include "VillagerCarryDiscovery.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace aoe2hd;
static uint32_t bits(float x){uint32_t raw;std::memcpy(&raw,&x,4);return raw;}
int main(){
 std::vector<CarryObservation> a(4);
 const float load[]={0,5,5,0};const bool stopped[]={false,false,true,false};
 const float other[]={2,2,0,0};
 for(int i=0;i<4;++i){
  a[i].hudLoad=load[i];a[i].stopped=stopped[i];
  a[i].sample.fields={{0,bits(load[i]),true},{4,bits(other[i]),true}};
 }
 auto c=RankCarryFields(a);
 assert(c.size()==2&&c[0].offset==0&&c[0].matched==4);
 assert(c[0].stoppedPairs==1&&c[0].preservedStopPairs==1&&c[0].perfect);
 assert(c[1].matched<4);
 a[2].sample.fields[0].readable=false;
 c=RankCarryFields(a);
 assert(c.size()==1&&c[0].offset==4);
 std::cout<<"Carry discovery tests passed\n";
}