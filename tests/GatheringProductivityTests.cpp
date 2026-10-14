#include "GatheringProductivity.h"
#include <cassert>
#include <cmath>
int main(){
 aoe2hd::GatheringProductivityTracker t;
 aoe2hd::CarryReading r;r.valid=true;r.object=100;r.id=2236;r.owner=20;r.definition=30;
 t.Observe(0,r);
 r.load=0.5f;auto s=t.Observe(250,r);
 assert(s.progressMs==250&&std::fabs(s.collected-0.5)<1e-5);
 s=t.Observe(500,r);assert(s.noProgressMs==250&&s.currentNoProgressMs==250);
 s=t.Observe(750,r);assert(s.longestNoProgressMs==500);
 r.load=1;s=t.Observe(1000,r);assert(s.currentNoProgressMs==0);
 assert(s.progressMs==500&&s.observedMs==1000);
 assert(std::fabs(s.ratePerActiveSecond-2.)<1e-5);
 r.valid=false;t.Observe(1250,r);
 r.valid=true;r.load=5;s=t.Observe(1500,r);assert(!s.comparable);
 assert(s.observedMs==1000);
 s=t.Observe(4000,r);assert(!s.comparable&&s.observedMs==1000);
 r.definition=31;s=t.Observe(4250,r);assert(!s.comparable);
 t.Reset();s=t.Observe(0,r);assert(s.observedMs==0);
 return 0;
}