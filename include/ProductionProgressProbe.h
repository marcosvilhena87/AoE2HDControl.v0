#pragma once
#include "UnitDiscovery.h"
#include <cstdint>
#include <string>
#include <vector>
namespace aoe2hd {
struct ProgressCandidate {
    std::string source;
    uint32_t vectorOffset=0,index=0,object=0,fieldOffset=0,raw=0;
    float value=0;
    const char* encoding="";
};
std::vector<ProgressCandidate> ScanProgressCandidates(
    const MemoryReader& memory,const std::vector<UnitVectorCandidate>& candidates,
    uint32_t maxObjects=24,uint32_t maxBytes=256);
}
