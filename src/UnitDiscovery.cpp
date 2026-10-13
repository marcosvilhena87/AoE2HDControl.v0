#include "UnitDiscovery.h"
#include <algorithm>
#include <limits>
namespace aoe2hd {
const char* UnitCandidateReasonName(UnitCandidateReason reason) {
    switch(reason) {
    case UnitCandidateReason::Accepted:return "Accepted";
    case UnitCandidateReason::Empty:return "Empty";
    case UnitCandidateReason::InvalidBounds:return "InvalidBounds";
    case UnitCandidateReason::Oversized:return "Oversized";
    case UnitCandidateReason::Misaligned:return "Misaligned";
    case UnitCandidateReason::UnreadableEntries:return "UnreadableEntries";
    case UnitCandidateReason::NoValidStride:return "NoValidStride";
    }
    return "Unknown";
}
UnitDiscoveryReport DiagnoseUnitVectors(const MemoryReader& memory,
    const WorldResult& world,uint32_t scanBytes,uint32_t maxEntries) {
    UnitDiscoveryReport report;
    const auto* local=GetLocalPlayer(world);
    if (!local || !local->object || !world.snapshot.world) return report;
    scanBytes=std::min(scanBytes,0x800u);
    maxEntries=std::min(maxEntries,1024u);
    struct Root {uint32_t addr;const char* name;};
    const Root roots[]={{local->object,"LocalPlayer"},{world.snapshot.world,"World"}};
    for(const auto& root:roots) {
        for(uint32_t offset=0x10;offset+12<=scanBytes;offset+=4) {
            ++report.scannedSlots;
            if(root.addr>UINT32_MAX-offset-12) {++report.readFailures;continue;}
            uint32_t w[3]{};
            if(!memory.read(root.addr+offset,w,sizeof(w))) {++report.readFailures;continue;}
            const uint32_t begin=w[0],end=w[1],cap=w[2];
            if (!begin || (begin&3)) {++report.nonVectorTriples;continue;}
            if (end<begin || cap<end || (end==begin && cap<begin)) {
                ++report.invalidBounds;continue;
            }
            if(cap-begin>8192 || end-begin>8192) {++report.oversized;continue;}
            if(end==begin) {
                ++report.emptyVectors;
                report.candidates.push_back({offset,begin,end,cap,0,0,0,root.name,UnitCandidateReason::Empty});
                continue;
            }
            bool validStride=false;
            for(uint32_t stride:{4u,8u}) {
                if((end-begin)%stride || (cap-begin)%stride) {
                    ++report.misaligned;continue;
                }
                uint32_t count=(end-begin)/stride;
                if (!count || count>maxEntries) {++report.oversized;continue;}
                validStride=true;
                uint32_t readable=0;
                for(uint32_t i=0;i<std::min(count,8u);++i) {
                    uint32_t entry=0;
                    if(memory.read(begin+i*stride,&entry,sizeof(entry)) && entry) ++readable;
                }
                const auto reason=readable>=std::min(count,2u) ?
                    UnitCandidateReason::Accepted:UnitCandidateReason::UnreadableEntries;
                if(reason==UnitCandidateReason::UnreadableEntries) ++report.unreadable;
                report.candidates.push_back({offset,begin,end,cap,count,stride,readable,root.name,reason});
            }
            if(!validStride) ++report.nonVectorTriples;
        }
    }
    return report;
}
std::vector<UnitVectorCandidate> DiscoverUnitVectors(const MemoryReader& memory,
    const WorldResult& world,uint32_t scanBytes,uint32_t maxEntries) {
    auto report=DiagnoseUnitVectors(memory,world,scanBytes,maxEntries);
    std::vector<UnitVectorCandidate> accepted;
    for(const auto& c:report.candidates)
        if(c.reason==UnitCandidateReason::Accepted)accepted.push_back(c);
    return accepted;
}
}
