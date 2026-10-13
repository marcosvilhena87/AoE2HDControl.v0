#include "ObjectFieldSampler.h"
#include <array>
#include <limits>
namespace aoe2hd {
std::vector<ObjectFieldChange> ObjectFieldSampler::observe(
    const MemoryReader& memory,const std::vector<UnitVectorCandidate>& candidates) {
    std::vector<ObjectFieldChange> changes;
    // Explicitly selected offsets, 8-byte entries; no speculative traversal.
    const std::array<uint32_t,3> watched{{0x88,0xA0,0xB0}};
    for(uint32_t offset:watched) {
        const UnitVectorCandidate* c=nullptr;
        for(const auto& v:candidates)
            if(std::string(v.source)=="World" && v.offset==offset && v.stride==8 &&
               v.reason==UnitCandidateReason::Accepted){c=&v;break;}
        if(!c)continue;
        const uint32_t samples=c->count<16?c->count:16;
        for(uint32_t index=0;index<samples;++index) {
            if(c->begin>UINT32_MAX-index*8)continue;
            uint32_t ptr=0;
            if(!memory.read(c->begin+index*8,&ptr,4) || ptr<0x10000u ||
               (ptr&3) || ptr>UINT32_MAX-64)continue;
            ObjectSample current;current.pointer=ptr;
            bool valid=true;
            for(uint32_t field=0;field<16;++field) {
                uint32_t word=0;
                if(!memory.read(ptr+field*4,&word,4)){valid=false;break;}
                current.fields.push_back(word);
            }
            if(!valid)continue;
            auto key=std::make_tuple(offset,index);
            auto it=previous_.find(key);
            if(it!=previous_.end()) {
                if(it->second.pointer!=ptr)
                    changes.push_back({"object_changed",offset,index,ptr,0,it->second.pointer,ptr});
                else for(uint32_t field=0;field<16;++field)
                    if(it->second.fields[field]!=current.fields[field])
                        changes.push_back({"field_changed",offset,index,ptr,field*4,
                            it->second.fields[field],current.fields[field]});
            }
            previous_[key]=current;
        }
    }
    return changes;
}
}
