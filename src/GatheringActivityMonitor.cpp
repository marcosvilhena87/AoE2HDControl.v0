#include "GatheringActivityMonitor.h"
#include <algorithm>
namespace aoe2hd {
const char* GatheringActivityName(GatheringActivity state){
 switch(state){
 case GatheringActivity::Unknown:return "activity_unknown";
 case GatheringActivity::ProgressRecent:return "recent_gathering_progress";
 case GatheringActivity::NoRecentProgress:return "no_recent_progress";
 case GatheringActivity::CapacityReached:return "capacity_reached";
 case GatheringActivity::LoadCleared:return "load_cleared_cause_unknown";
 }return "activity_unknown";
}
GatheringActivityMonitor::GatheringActivityMonitor(uint64_t ms,float epsilon,float cap):
 windowMs_(ms?ms:1),epsilon_(epsilon>0?epsilon:0.0001f),capacity_(cap>0?cap:10.f){}
void GatheringActivityMonitor::Reset(){points_.clear();hasLast_=false;last_={};}
GatheringActivityResult GatheringActivityMonitor::Observe(uint64_t time,const CarryReading& now){
 GatheringActivityResult r;
 if(!now.valid){Reset();return r;}
 if(hasLast_&&(now.object!=last_.object||now.id!=last_.id||
   now.owner!=last_.owner||now.definition!=last_.definition||
   (!points_.empty()&&time<=points_.back().time))){Reset();}
 const bool cleared=hasLast_&&last_.load>epsilon_&&now.load<=epsilon_;
 // Keep the first sample at/before the left window boundary to establish coverage.
 while(points_.size()>1&&points_[1].time+windowMs_<=time)points_.pop_front();
 if(!points_.empty()&&time<points_.front().time){Reset();}
 points_.push_back({time,now.load});
 hasLast_=true;last_=now;
 if(cleared){points_.clear();points_.push_back({time,now.load});r.state=GatheringActivity::LoadCleared;return r;}
 r.completeWindow=points_.size()>1&&time-points_.front().time>=windowMs_;
 r.windowMs=points_.size()>1?time-points_.front().time:0;
 for(size_t i=1;i<points_.size();++i)
  r.windowGain+=std::max(0.f,points_[i].load-points_[i-1].load);
 if(r.windowGain>epsilon_)r.state=GatheringActivity::ProgressRecent;
 else if(now.load>=capacity_-epsilon_)r.state=GatheringActivity::CapacityReached;
 else if(r.completeWindow)r.state=GatheringActivity::NoRecentProgress;
 return r;
}
}