#include "ProductionProgressProbe.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
namespace aoe2hd {
std::vector<ProgressCandidate> ScanProgressCandidates(
    const MemoryReader& memory,const std::vector<UnitVectorCandidate>& candidates,
    uint32_t maxObjects,uint32_t maxBytes) {
    std::vector<ProgressCandidate> output;
    maxObjects=std::min(maxObjects,24u);
    maxBytes=std::min(maxBytes,256u)&~3u;
    for(uint32_t offset:{0x88u,0xA0u,0xB0u}) {
        const UnitVectorCandidate* c=nullptr;
        for(const auto& v:candidates)
            if(std::string(v.source)=="World" && v.offset==offset &&
                v.reason==UnitCandidateReason::Accepted &&
                (v.stride==4 || v.stride==8) &&
                (!c || v.stride==(offset==0xA0 ? 4u : 8u))) c=&v;
        if(!c || !c->count)continue;
        const uint32_t n=std::min(c->count,maxObjects);
        for(uint32_t slot=0;slot<n;++slot) {
            const uint32_t index=n==1?0:static_cast<uint32_t>(
                static_cast<uint64_t>(slot)*(c->count-1)/(n-1));
            if(c->begin>UINT32_MAX-index*c->stride)continue;
            uint32_t object=0;
            if(!memory.read(c->begin+index*c->stride,&object,4) ||
                object<0x10000 || (object&3) || object>UINT32_MAX-maxBytes)continue;
            // Read one bounded block, never follow pointers found inside.
            std::vector<uint32_t> words(maxBytes/4);
            if(!memory.read(object,words.data(),maxBytes))continue;
            for(uint32_t field=4;field<maxBytes;field+=4) {
                const uint32_t raw=words[field/4];
                float number=0;
                std::memcpy(&number,&raw,4);
                if(std::isfinite(number) && number>0.0001f && number<1.0f)
                    output.push_back({"World",offset,index,object,field,raw,number,"fraction"});
                else if(std::isfinite(number) && number>=1.0f && number<=100.0f)
                    output.push_back({"World",offset,index,object,field,raw,number,"percent"});
                if(raw>0 && raw<=100)
                    output.push_back({"World",offset,index,object,field,raw,
                        static_cast<float>(raw),"integer"});
            }
        }
    }
    return output;
}
}
