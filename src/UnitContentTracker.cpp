#include "UnitContentTracker.h"
#include <algorithm>
#include <limits>
namespace aoe2hd {
std::vector<UnitContentChange> UnitContentTracker::observe(
    const MemoryReader& memory,const std::vector<UnitVectorCandidate>& candidates) {
    std::vector<UnitContentChange> changes;
    // A candidate with incompatible content is omitted; partial reads never change the baseline.
    for(const auto& c:candidates) {
        if(c.reason!=UnitCandidateReason::Accepted || !c.stride ||
           c.count>512 || c.count==0 || (c.stride!=4 && c.stride!=8) ||
           c.begin>UINT32_MAX-c.count*c.stride) continue;
        Snapshot fresh;fresh.begin=c.begin;
        bool ok=true;
        for(uint32_t i=0;i<c.count;++i) {
            uint32_t value=0;
            if(!memory.read(c.begin+i*c.stride,&value,4)){ok=false;break;}
            fresh.entries.push_back(value);
        }
        if(!ok) continue;
        auto key=std::make_tuple(std::string(c.source),c.offset,c.stride);
        auto old=previous_.find(key);
        if(old!=previous_.end()) {
            const auto& prior=old->second;
            if(prior.begin==fresh.begin) {
                const auto common=std::min(prior.entries.size(),fresh.entries.size());
                for(size_t i=0;i<common;++i) {
                    if(prior.entries[i]==fresh.entries[i])continue;
                    changes.push_back({c.source,"changed",c.offset,c.stride,
                        static_cast<uint32_t>(i),prior.entries[i],fresh.entries[i],c.count});
                }
                for(size_t i=common;i<fresh.entries.size();++i)
                    changes.push_back({c.source,"added",c.offset,c.stride,
                        static_cast<uint32_t>(i),0,fresh.entries[i],c.count});
                for(size_t i=common;i<prior.entries.size();++i)
                    changes.push_back({c.source,"removed",c.offset,c.stride,
                        static_cast<uint32_t>(i),prior.entries[i],0,c.count});
            } else {
                changes.push_back({c.source,"relocated",c.offset,c.stride,0,
                    prior.begin,fresh.begin,c.count});
            }
        }
        previous_[key]=std::move(fresh);
    }
    return changes;
}
}
