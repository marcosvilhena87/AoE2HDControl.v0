#include "ObjectFieldSampler.h"
#include <algorithm>
#include <array>
#include <limits>
namespace aoe2hd {
std::vector<ObjectFieldChange> ObjectFieldSampler::observe(
    const MemoryReader& memory,const std::vector<UnitVectorCandidate>& candidates) {
    std::vector<ObjectFieldChange> changes;
    summary_.clear();
    const std::array<uint32_t,3> watched{{0x88,0xA0,0xB0}};
    for(uint32_t offset:watched) {
        ObjectFieldSummary summary;summary.offset=offset;
        const UnitVectorCandidate* c=nullptr;
        for(const auto& v:candidates)
            if(std::string(v.source)=="World" && v.offset==offset &&
                v.reason==UnitCandidateReason::Accepted &&
                (v.stride==8 || (offset==0xA0 && v.stride==4))) {
                if(!c || v.stride==8)c=&v;
            }
        if(!c) {summary_[offset]=summary;continue;}
        const uint32_t samples=std::min(c->count,24u);
        // Evenly spaced fixed indexes across the vector, not just first entries.
        for(uint32_t slot=0;slot<samples;++slot) {
            const uint32_t index=samples==1?0:
                static_cast<uint32_t>((static_cast<uint64_t>(slot)*(c->count-1))/(samples-1));
            if(c->begin>UINT32_MAX-index*c->stride) {++summary.readFailures;continue;}
            uint32_t ptr=0;
            if(!memory.read(c->begin+index*c->stride,&ptr,4) ||
               ptr<0x10000u || (ptr&3) || ptr>UINT32_MAX-128) {
                ++summary.readFailures;continue;
            }
            ObjectSample current;current.pointer=ptr;
            bool valid=true;
            for(uint32_t field=0;field<32;++field) {
                uint32_t word=0;
                if(!memory.read(ptr+field*4,&word,4)){valid=false;break;}
                current.fields.push_back(word);
            }
            if(!valid){++summary.readFailures;continue;}
            ++summary.objects;
            summary.fields+=32;
            auto key=std::make_tuple(offset,index);
            auto it=previous_.find(key);
            if(it!=previous_.end()) {
                if(it->second.pointer!=ptr) {
                    changes.push_back({"object_changed",offset,index,ptr,0,it->second.pointer,ptr});
                    ++summary.changes;
                } else for(uint32_t field=0;field<32;++field)
                    if(it->second.fields[field]!=current.fields[field]) {
                        changes.push_back({"field_changed",offset,index,ptr,field*4,
                            it->second.fields[field],current.fields[field]});
                        ++summary.changes;
                    }
            }
            previous_[key]=std::move(current);
        }
        summary_[offset]=summary;
    }
    return changes;
}
std::vector<ObjectFieldSummary> ObjectFieldSampler::summaries() const {
    std::vector<ObjectFieldSummary> out;
    for(const auto& s:summary_)out.push_back(s.second);
    return out;
}
}
