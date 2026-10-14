#include "GatheringActivityMonitor.h"
#include <cassert>
#include <iostream>
using namespace aoe2hd;
static CarryReading read(float load,uint32_t id=2236,uint32_t def=5125){
 CarryReading r;r.valid=true;r.object=345654168;r.id=id;r.owner=10;r.definition=def;r.load=load;return r;
}
int main(){
 GatheringActivityMonitor m(2000);
 assert(m.Observe(0,read(0)).state==GatheringActivity::Unknown);
 assert(m.Observe(1000,read(0.5f)).state==GatheringActivity::ProgressRecent);
 assert(m.Observe(2000,read(0.5f)).state==GatheringActivity::ProgressRecent);
 auto t=m.Observe(3100,read(0.5f));
 assert(!t.completeWindow||t.state==GatheringActivity::Unknown||t.state==GatheringActivity::NoRecentProgress);
 // Sampling continuously across two seconds establishes genuine no-progress evidence.
 m.Reset();m.Observe(0,read(4));m.Observe(1000,read(4));
 assert(m.Observe(2100,read(4)).state==GatheringActivity::NoRecentProgress);
 assert(m.Observe(2400,read(4.2f)).state==GatheringActivity::ProgressRecent);
 assert(m.Observe(2600,read(0)).state==GatheringActivity::LoadCleared);
 assert(m.Observe(2700,read(0)).state==GatheringActivity::Unknown);
 m.Reset();m.Observe(0,read(10));
 assert(m.Observe(2100,read(10)).state==GatheringActivity::CapacityReached);
 assert(m.Observe(2500,read(5,2234)).state==GatheringActivity::Unknown);
 assert(m.Observe(3000,read(5,2234,5482)).state==GatheringActivity::Unknown);
 assert(m.Observe(3100,{}).state==GatheringActivity::Unknown);
 std::cout<<"Gathering activity tests passed\n";
}