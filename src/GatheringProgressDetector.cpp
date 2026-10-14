#include "GatheringProgressDetector.h"
#include "SelectedTelemetryReader.h"
#include <cmath>
#include <cstring>
namespace aoe2hd {
CarryReading ReadSelectedVillagerCarry(const MemoryReader& memory,const WorldResult& world){
 CarryReading result;
 auto s=ReadSelectedTelemetry(memory,world);
 if(s.status!=TelemetryStatus::Ready||s.ownership!=EntityOwnership::Local||
    !s.identity.candidateId||s.object>UINT32_MAX-0x48u)return result;
 uint32_t raw=0,confirm=0;
 if(!memory.read(s.object+0x48u,&raw,4)||
    !memory.read(s.object+0x08u,&confirm,4)||
    confirm!=s.identity.candidateId)return result;
 auto recheck=ReadSelectedTelemetry(memory,world);
 if(recheck.status!=TelemetryStatus::Ready||recheck.object!=s.object||
    recheck.identity.candidateId!=s.identity.candidateId||
    recheck.identity.candidateOwner!=s.identity.candidateOwner||
    recheck.identity.candidateTypeRef!=s.identity.candidateTypeRef)return result;
 float load=0;std::memcpy(&load,&raw,4);
 // Range is a plausibility guard, not a hardcoded 10-resource capacity.
 if(!std::isfinite(load)||load<0||load>1000)return result;
 result.valid=true;result.object=s.object;result.id=s.identity.candidateId;
 result.owner=s.identity.candidateOwner;result.definition=s.identity.candidateTypeRef;
 result.raw=raw;result.load=load;return result;
}
GatheringProgress CompareGatheringProgress(const CarryReading& a,const CarryReading& b,float epsilon){
 if(!b.valid)return GatheringProgress::Unreadable;
 if(!a.valid)return GatheringProgress::Baseline;
 if(a.object!=b.object||a.id!=b.id||a.owner!=b.owner||
    a.definition!=b.definition)return GatheringProgress::IdentityChanged;
 float delta=b.load-a.load;
 if(delta>epsilon)return GatheringProgress::Increased;
 if(delta< -epsilon){
  if(b.load<=epsilon)return GatheringProgress::Cleared;
  return GatheringProgress::Decreased;
 }
 return GatheringProgress::Unchanged;
}
const char* GatheringProgressName(GatheringProgress p){
 switch(p){
 case GatheringProgress::Baseline:return "baseline";
 case GatheringProgress::Increased:return "gathering_progress";
 case GatheringProgress::Unchanged:return "no_progress_observed";
 case GatheringProgress::Decreased:return "load_decreased";
 case GatheringProgress::Cleared:return "load_cleared_cause_unknown";
 case GatheringProgress::IdentityChanged:return "identity_or_role_changed";
 case GatheringProgress::Unreadable:return "unreadable";
 }return "unknown";
}
}