#include "UnitCollectionDiscovery.h"
namespace aoe2hd {
UnitCollectionReport InspectUnitCollections(
 const MemoryReader& memory,const WorldResult& world,
 uint32_t selectedObject,uint32_t scanBytes,
 uint32_t maxEntries,uint32_t sampleLimit){
 UnitCollectionReport report;
 report.discovery=DiagnoseUnitVectors(memory,world,scanBytes,maxEntries);
 std::vector<UnitVectorCandidate> accepted;
 for(const auto& candidate:report.discovery.candidates)
  if(candidate.reason==UnitCandidateReason::Accepted)
   accepted.push_back(candidate);
 auto ranked=ProbePlayerUnitLists(memory,world,accepted,selectedObject,sampleLimit);
 report.observations.reserve(ranked.size());
 for(auto& summary:ranked){
  UnitCollectionObservation observation;
  observation.entries=ProbeUnitEntryEvidence(memory,world,summary.vector,selectedObject);
  observation.summary=std::move(summary);
  report.observations.push_back(std::move(observation));
 }
 return report;
}
}
