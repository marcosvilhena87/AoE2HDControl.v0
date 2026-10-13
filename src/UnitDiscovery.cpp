#include "UnitDiscovery.h"
#include <algorithm>
#include <limits>
namespace aoe2hd {
std::vector<UnitVectorCandidate> DiscoverUnitVectors(
    const MemoryReader& memory, const WorldResult& world,
    uint32_t scanBytes, uint32_t maxEntries) {
    std::vector<UnitVectorCandidate> results;
    const auto* local=GetLocalPlayer(world);
    if (!local || !local->object) return results;
    scanBytes=std::min(scanBytes,0x800u);
    maxEntries=std::min(maxEntries,1024u);
    // Scan candidate 3-pointer vectors, with both common 4- and 8-byte entry layouts.
    for (uint32_t offset=0x10;offset+12<=scanBytes;offset+=4) {
        if (local->object>UINT32_MAX-offset-12) break;
        uint32_t words[3]{};
        if (!memory.read(local->object+offset,words,sizeof(words))) continue;
        const uint32_t begin=words[0],end=words[1],cap=words[2];
        if (!begin || (begin&3) || end<=begin || cap<end || cap-begin>8192) continue;
        for (uint32_t stride : {4u,8u}) {
            if ((end-begin)%stride || (cap-begin)%stride) continue;
            const uint32_t count=(end-begin)/stride;
            if (!count || count>maxEntries) continue;
            uint32_t readable=0;
            // Verify a bounded sample of entries, without dereferencing untrusted pointers.
            for (uint32_t i=0;i<std::min(count,8u);++i) {
                uint32_t entry=0;
                if (memory.read(begin+i*stride,&entry,sizeof(entry)) && entry)
                    ++readable;
            }
            if (readable>=std::min(count,2u))
                results.push_back({offset,begin,end,cap,count,stride,readable});
        }
    }
    return results;
}
}
