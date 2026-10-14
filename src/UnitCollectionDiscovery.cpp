#include "UnitCollectionDiscovery.h"
#include <utility>
namespace aoe2hd {
UnitCollectionReport InspectUnitCollections(
 const MemoryReader& memory,const WorldResult& world,
 uint32_t selectedObject,uint32_t scanBytes,
 uint32_t maxEntries,uint32_t sampleLimit,uint32_t moduleBase){
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
  // Probe one vector interpretation at a time so stride variants are not conflated.
  const auto evidence=ProbeUnitPointers(memory,world,{summary.vector},moduleBase,sampleLimit);
  if(!evidence.empty())observation.pointers=evidence.front();
  observation.summary=std::move(summary);
  report.observations.push_back(std::move(observation));
 }
 return report;
}
}
