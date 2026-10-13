#include "UnitPointerProbe.h"
#include <algorithm>
#include <set>
#include <tuple>
namespace aoe2hd {
std::vector<UnitPointerEvidence> ProbeUnitPointers(
    const MemoryReader& memory,const WorldResult& world,
    const std::vector<UnitVectorCandidate>& candidates,
    uint32_t moduleBase,uint32_t maxSamples) {
    std::vector<UnitPointerEvidence> out;
    if(!world.ready())return out;
    maxSamples=std::min(maxSamples,64u);
    // One representation per (source,offset,begin). Prefer 8-byte entries
    // when a known vector's end aligns to 8; this is only a heuristic.
    std::set<std::tuple<std::string,uint32_t,uint32_t>> seen;
    for(auto it=candidates.rbegin();it!=candidates.rend();++it) {
        const auto& c=*it;
        if(c.reason!=UnitCandidateReason::Accepted || !c.count ||
           (c.stride!=4 && c.stride!=8))continue;
        auto key=std::make_tuple(std::string(c.source),c.offset,c.begin);
        if(!seen.insert(key).second)continue;
        UnitPointerEvidence e;
        e.source=c.source;e.offset=c.offset;e.stride=c.stride;
        e.count=c.count;e.begin=c.begin;
        std::set<uint32_t> targets;
        std::map<uint32_t,uint32_t> vtables;
        const uint32_t count=std::min(c.count,maxSamples);
        for(uint32_t i=0;i<count;++i) {
            uint32_t ptr=0;
            if(!memory.read(c.begin+i*c.stride,&ptr,4))continue;
            ++e.sampled;
            if(!ptr)continue;
            ++e.nonzero;
            if(ptr<0x10000u || (ptr&3u))continue;
            uint32_t signature=0;
            if(!memory.read(ptr,&signature,4))continue;
            ++e.readableTargets;
            targets.insert(ptr);
            if(moduleBase && signature>=moduleBase &&
               static_cast<uint64_t>(signature)-moduleBase<0x00800000u &&
               (signature&3u)==0) {
                ++e.plausibleVtables;
                ++vtables[signature];
            }
        }
        e.distinctTargets=static_cast<uint32_t>(targets.size());
        for(const auto& item:vtables)
            if(item.second>e.matchingVtables) {
                e.firstVtable=item.first;
                e.matchingVtables=item.second;
            }
        out.push_back(e);
    }
    std::reverse(out.begin(),out.end());
    return out;
}
}
