#include "VillagerSelectionGuard.h"
#include <cassert>
#include <iostream>
using namespace aoe2hd;
int main(){
 SelectedTelemetry s;s.status=TelemetryStatus::Ready;s.identity.readable=true;
 s.identity.candidateId=2236;s.ownership=EntityOwnership::Local;
 s.typeCandidatesReadable=true;s.typeCandidateA=5125;
 assert(CheckVillagerCandidate(s)==VillagerSelectionStatus::AcceptedCandidate);
 s.typeCandidateA=5164;
 assert(CheckVillagerCandidate(s)==VillagerSelectionStatus::NotKnownVillagerType);
 s.typeCandidateA=5482;
 assert(CheckVillagerCandidate(s)==VillagerSelectionStatus::AcceptedCandidate);
 s.typeCandidatesReadable=false;
 assert(CheckVillagerCandidate(s)==VillagerSelectionStatus::MissingType);
 s.typeCandidatesReadable=true;s.ownership=EntityOwnership::OtherPlayer;
 assert(CheckVillagerCandidate(s)==VillagerSelectionStatus::NotLocal);
 s.status=TelemetryStatus::None;
 assert(CheckVillagerCandidate(s)==VillagerSelectionStatus::NoSelection);
 std::cout<<"Villager selection guard tests passed\n";
}