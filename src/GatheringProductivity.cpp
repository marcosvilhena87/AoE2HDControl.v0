#include "GatheringProductivity.h"
#include <algorithm>
namespace aoe2hd {
void GatheringProductivityTracker::Reset(){previous_={};previousTime_=0;hasPrevious_=false;totals_={};}
GatheringProductivity GatheringProductivityTracker::Observe(uint64_t time,const CarryReading& c){
 totals_.comparable=false;
 if(!c.valid){hasPrevious_=false;totals_.currentNoProgressMs=0;return totals_;}
 if(hasPrevious_){
  if(time<=previousTime_||time-previousTime_>1000||
    c.id!=previous_.id||c.object!=previous_.object||
    c.owner!=previous_.owner||c.definition!=previous_.definition){
   hasPrevious_=false;totals_.currentNoProgressMs=0;
  }
 }
 if(hasPrevious_){
  const auto elapsed=time-previousTime_;
  const float delta=c.load-previous_.load;
  totals_.comparable=true;
  totals_.observedMs+=elapsed;
  if(delta>0.0001f){
   totals_.progressMs+=elapsed;totals_.collected+=delta;
   totals_.currentNoProgressMs=0;
  }else{
   totals_.noProgressMs+=elapsed;
   totals_.currentNoProgressMs+=elapsed;
   totals_.longestNoProgressMs=std::max(totals_.longestNoProgressMs,totals_.currentNoProgressMs);
  }
  totals_.progressFraction=totals_.observedMs?
   static_cast<double>(totals_.progressMs)/totals_.observedMs:0;
  totals_.ratePerActiveSecond=totals_.progressMs?
   totals_.collected*1000.0/totals_.progressMs:0;
 }
 previous_=c;previousTime_=time;hasPrevious_=true;
 return totals_;
}
}