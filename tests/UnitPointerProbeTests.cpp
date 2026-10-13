#include "UnitPointerProbe.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake : MemoryReader {
    std::map<uint32_t,uint32_t> words;
    bool read(uint32_t addr,void* dst,size_t n) const override {
        if(n!=4 || !words.count(addr))return false;
        std::memcpy(dst,&words.at(addr),4);return true;
    }
};
int main() {
    Fake m;WorldResult w;w.status=WorldStatus::Ready;w.snapshot.world=0x5000;
    UnitVectorCandidate a;a.source="World";a.offset=0x88;a.begin=0x6000;
    a.stride=4;a.count=3;a.reason=UnitCandidateReason::Accepted;
    m.words[0x6000]=0x7000;m.words[0x6004]=0x7100;m.words[0x6008]=0;
    m.words[0x7000]=0x405000;m.words[0x7100]=0x405000;
    auto result=ProbeUnitPointers(m,w,{a},0x400000);
    assert(result.size()==1 && result[0].sampled==3);
    assert(result[0].readableTargets==2 && result[0].plausibleVtables==2);
    assert(result[0].matchingVtables==2);
    auto b=a;b.stride=8;b.count=1;
    result=ProbeUnitPointers(m,w,{a,b},0x400000);
    assert(result.size()==1); // overlapping interpretation suppressed
    w.status=WorldStatus::InvalidVector;
    assert(ProbeUnitPointers(m,w,{a},0x400000).empty());
    std::cout<<"Unit pointer probe tests passed\n";
}
