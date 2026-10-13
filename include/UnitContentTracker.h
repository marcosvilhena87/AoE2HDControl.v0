#pragma once
#include "UnitDiscovery.h"
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>
namespace aoe2hd {
struct UnitContentChange {
    std::string source,kind;
    uint32_t offset=0,stride=0,index=0,before=0,after=0,count=0;
};
class UnitContentTracker {
public:
    std::vector<UnitContentChange> observe(const MemoryReader& memory,
        const std::vector<UnitVectorCandidate>& candidates);
    void reset() { previous_.clear(); }
private:
    struct Snapshot {uint32_t begin=0;std::vector<uint32_t> entries;};
    std::map<std::tuple<std::string,uint32_t,uint32_t>,Snapshot> previous_;
};
}
