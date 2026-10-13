#pragma once
#include "EntityIdentityReader.h"
namespace aoe2hd {
enum class TelemetryStatus { WorldUnavailable, SlotReadFailed, None, MultipleOrTransition, InvalidObject, IdentityReadFailed, HpReadFailed, HpInvalid, ChangedDuringRead, Ready };
struct SelectedTelemetry {
 TelemetryStatus status=TelemetryStatus::WorldUnavailable;
 uint32_t slot15c=0,slot160=0,object=0,hpRaw=0;
 float hp=0;
 EntityIdentity identity;
 uint32_t typeCandidateA=0,typeCandidateB=0;
 bool typeCandidatesReadable=false,typeReferenceStable=false;
 EntityOwnership ownership=EntityOwnership::Unknown;
};
SelectedTelemetry ReadSelectedTelemetry(const MemoryReader& memory,const WorldResult& world);
const char* TelemetryStatusName(TelemetryStatus status);
}