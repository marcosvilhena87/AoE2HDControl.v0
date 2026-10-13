#include "SelectedTelemetryReader.h"
#include <cmath>
#include <cstring>
namespace aoe2hd {
SelectedTelemetry ReadSelectedTelemetry(const MemoryReader& memory,const WorldResult& world){
 SelectedTelemetry s;
 const auto* player=GetLocalPlayer(world);
 if(!player||player->object<0x10000u)return s;
 if(player->object>UINT32_MAX-0x160u){s.status=TelemetryStatus::SlotReadFailed;return s;}
 if(!memory.read(player->object+0x15c,&s.slot15c,4)||
    !memory.read(player->object+0x160,&s.slot160,4)){
  s.status=TelemetryStatus::SlotReadFailed;return s;
 }
 if(!s.slot15c&&!s.slot160){s.status=TelemetryStatus::None;return s;}
 if(s.slot15c!=s.slot160){s.status=TelemetryStatus::MultipleOrTransition;return s;}
 s.object=s.slot15c;
 if(s.object<0x10000u || (s.object&3u) || s.object>UINT32_MAX-0x34u){
  s.status=TelemetryStatus::InvalidObject;return s;
 }
 s.identity=ReadEntityIdentity(memory,s.object);
 if(!s.identity.readable){s.status=TelemetryStatus::IdentityReadFailed;return s;}
 if(!memory.read(s.object+0x34,&s.hpRaw,4)){
  s.status=TelemetryStatus::HpReadFailed;return s;
 }
 std::memcpy(&s.hp,&s.hpRaw,4);
 if(!std::isfinite(s.hp)||s.hp<0||s.hp>100000){
  s.status=TelemetryStatus::HpInvalid;return s;
 }
 uint32_t a=0,b=0;
 if(!memory.read(player->object+0x15c,&a,4)||
    !memory.read(player->object+0x160,&b,4)||
    a!=s.slot15c||b!=s.slot160){
  s.status=TelemetryStatus::ChangedDuringRead;return s;
 }
 s.ownership=ClassifyEntityOwnership(s.identity,world);
 s.status=TelemetryStatus::Ready;
 return s;
}
const char* TelemetryStatusName(TelemetryStatus status){
 switch(status){
 case TelemetryStatus::WorldUnavailable:return "world_unavailable";
 case TelemetryStatus::SlotReadFailed:return "slot_read_failed";
 case TelemetryStatus::None:return "none";
 case TelemetryStatus::MultipleOrTransition:return "multiple_or_transition";
 case TelemetryStatus::InvalidObject:return "invalid_object";
 case TelemetryStatus::IdentityReadFailed:return "identity_read_failed";
 case TelemetryStatus::HpReadFailed:return "hp_read_failed";
 case TelemetryStatus::HpInvalid:return "hp_invalid";
 case TelemetryStatus::ChangedDuringRead:return "changed_during_read";
 case TelemetryStatus::Ready:return "ready";
 }
 return "unknown";
}
}