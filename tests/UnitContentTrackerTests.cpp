#include "UnitContentTracker.h"
#include <cassert>
#include <cstdint>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake:MemoryReader {
    std::map<uint32_t,uint32_t> words;
    bool read(uint32_t a,void* p,size_t n) const override {
        if(n!=4 || !words.count(a))return false;
        *static_cast<uint32_t*>(p)=words.at(a);return true;
    }
};
int main() {
    Fake m;
    UnitContentTracker t;
    UnitVectorCandidate c;
    c.source="World";c.offset=0x88;c.begin=0x4000;c.stride=4;
    c.count=2;c.reason=UnitCandidateReason::Accepted;
    m.words[0x4000]=1;m.words[0x4004]=2;
    assert(t.observe(m,{c}).empty());
    m.words[0x4004]=3;
    auto v=t.observe(m,{c});
    assert(v.size()==1 && v[0].kind=="changed" && v[0].index==1);
    c.count=3;m.words[0x4008]=4;
    v=t.observe(m,{c});assert(v.size()==1 && v[0].kind=="added");
    m.words.erase(0x4008);
    assert(t.observe(m,{c}).empty());
    c.begin=0x5000;c.count=1;m.words[0x5000]=9;
    v=t.observe(m,{c});assert(v.size()==1 && v[0].kind=="relocated");
    t.reset();assert(t.observe(m,{c}).empty());
    std::cout<<"Unit content tracker tests passed\n";
}
