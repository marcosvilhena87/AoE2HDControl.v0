#include "CrossPlayerTypeExperiment.h"
namespace aoe2hd {
CrossPlayerTypeRecord CaptureCrossPlayerType(const MemoryReader& memory,
 const WorldResult& world,const std::string& label,const std::string& kind,uint32_t object){
 CrossPlayerTypeRecord r;r.label=label;r.kind=kind;r.object=object;
 r.sample=SampleTypeReference(memory,object,label,0x10);
 r.ownership=ClassifyEntityOwnership(r.sample.identity,world);
 if(!r.sample.stable||r.sample.definition.fields.size()<4)return r;
 const auto& fields=r.sample.definition.fields;
 r.candidateAReadable=fields[2].readable;
 r.candidateBReadable=fields[3].readable;
 if(r.candidateAReadable)r.candidateA=fields[2].raw;
 if(r.candidateBReadable)r.candidateB=fields[3].raw;
 return r;
}
PairVerdict CompareCrossPlayerType(const CrossPlayerTypeRecord& a,
 const CrossPlayerTypeRecord& b,bool useB){
 const bool validA=a.sample.stable&&(useB?a.candidateBReadable:a.candidateAReadable);
 const bool validB=b.sample.stable&&(useB?b.candidateBReadable:b.candidateAReadable);
 if(!validA||!validB)return PairVerdict::Insufficient;
 const uint32_t av=useB?a.candidateB:a.candidateA;
 const uint32_t bv=useB?b.candidateB:b.candidateA;
 return av==bv?PairVerdict::Equal:PairVerdict::Different;
}
const char* PairVerdictName(PairVerdict value){
 switch(value){
 case PairVerdict::Insufficient:return "insufficient";
 case PairVerdict::Equal:return "equal";
 case PairVerdict::Different:return "different";
 }
 return "insufficient";
}
}