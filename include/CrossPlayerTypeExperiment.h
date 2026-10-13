#pragma once
#include "EntityTypeResolver.h"
#include <cstdint>
#include <string>
#include <vector>
namespace aoe2hd {
struct CrossPlayerTypeRecord {
 std::string label,kind;
 uint32_t object=0;
 EntityOwnership ownership=EntityOwnership::Unknown;
 TypeReferenceSample sample;
 bool candidateAReadable=false,candidateBReadable=false;
 uint32_t candidateA=0,candidateB=0;
};
CrossPlayerTypeRecord CaptureCrossPlayerType(const MemoryReader& memory,
 const WorldResult& world,const std::string& label,const std::string& kind,uint32_t object);
enum class PairVerdict { Insufficient, Equal, Different };
PairVerdict CompareCrossPlayerType(const CrossPlayerTypeRecord& a,const CrossPlayerTypeRecord& b,bool useB);
const char* PairVerdictName(PairVerdict verdict);
}