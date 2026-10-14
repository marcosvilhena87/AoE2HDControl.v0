#include "VillagerSelectionGuard.h"
namespace aoe2hd {
VillagerSelectionStatus CheckVillagerCandidate(const SelectedTelemetry& s){
 if(s.status!=TelemetryStatus::Ready||!s.identity.readable||!s.identity.candidateId)
  return VillagerSelectionStatus::NoSelection;
 if(s.ownership!=EntityOwnership::Local)return VillagerSelectionStatus::NotLocal;
 if(!s.typeCandidatesReadable)return VillagerSelectionStatus::MissingType;
 // Empirical candidate definition IDs observed with villager HUD roles in this build.
 // Deliberately fail closed: other villager/civilization IDs require validation.
 switch(s.typeCandidateA){
 case 5121:case 5122:case 5123:case 5124:case 5125:case 5126:
 case 5402:case 5482:case 5496:case 5499:
  return VillagerSelectionStatus::AcceptedCandidate;
 default:return VillagerSelectionStatus::NotKnownVillagerType;
 }
}
const char* VillagerSelectionStatusName(VillagerSelectionStatus s){
 switch(s){
 case VillagerSelectionStatus::AcceptedCandidate:return "accepted_villager_candidate";
 case VillagerSelectionStatus::NoSelection:return "no_valid_single_selection";
 case VillagerSelectionStatus::NotLocal:return "not_local";
 case VillagerSelectionStatus::MissingType:return "definition_unreadable";
 case VillagerSelectionStatus::NotKnownVillagerType:return "unrecognized_villager_definition";
 }return "unknown";
}
}