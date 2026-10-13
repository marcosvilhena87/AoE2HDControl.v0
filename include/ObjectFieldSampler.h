#pragma once
#include "UnitDiscovery.h"
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>
namespace aoe2hd {
struct ObjectFieldChange {
    std::string kind;
    uint32_t offset=0,index=0,object=0,field=0,before=0,after=0;
};
struct ObjectFieldSummary {
    uint32_t offset=0,objects=0,fields=0,changes=0,readFailures=0;
};
class ObjectFieldSampler {
public:
    std::vector<ObjectFieldChange> observe(const MemoryReader& memory,
        const std::vector<UnitVectorCandidate>& candidates);
    std::vector<ObjectFieldSummary> summaries() const;
    void reset(){previous_.clear();summary_.clear();}
private:
    struct ObjectSample {uint32_t pointer=0;std::vector<uint32_t> fields;};
    std::map<std::tuple<uint32_t,uint32_t>,ObjectSample> previous_;
    std::map<uint32_t,ObjectFieldSummary> summary_;
};
}
