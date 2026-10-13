#include "UnitDiscovery.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
using namespace aoe2hd;
struct Fake : MemoryReader {
    std::map<uint32_t,uint8_t> bytes;
    void put(uint32_t addr,uint32_t v) {
        for(unsigned i=0;i<4;i++)bytes[addr+i]=static_cast<uint8_t>(v>>(8*i));
    }
    bool read(uint32_t addr,void* dst,size_t n) const override {
        auto* b=static_cast<uint8_t*>(dst);
        for(size_t i=0;i<n;i++) {
            auto it=bytes.find(addr+static_cast<uint32_t>(i));
            if(it==bytes.end())return false;
            b[i]=it->second;
        }
        return true;
    }
};
int main(){
    Fake m;
    WorldResult w;
    w.status=WorldStatus::Ready;
    w.snapshot.localIndex=1;
    w.snapshot.world=0x2000;
    w.snapshot.players.resize(2);
    w.snapshot.players[1].object=0x1000;
    m.put(0x1020,0x4000);m.put(0x1024,0x400c);m.put(0x1028,0x4010);
    m.put(0x4000,0x5000);m.put(0x4004,0x5010);m.put(0x4008,0x5020);
    m.put(0x2030,0x6000);m.put(0x2034,0x6000);m.put(0x2038,0x6000);
    auto report=DiagnoseUnitVectors(m,w,0x40);
    assert(report.scannedSlots>0);
    bool emptyWorld=false;
    for(const auto& c:report.candidates)
        if(std::string(c.source)=="World" && c.offset==0x30 && c.reason==UnitCandidateReason::Empty)emptyWorld=true;
    assert(emptyWorld);
    auto found=DiscoverUnitVectors(m,w,0x40);
    assert(!found.empty());
    bool match=false;
    for(const auto& c:found)if(c.offset==0x20 && c.stride==4 && c.count==3)match=true;
    assert(match);
    w.status=WorldStatus::InvalidPlayer;
    assert(DiscoverUnitVectors(m,w).empty());
    std::cout<<"Unit discovery tests passed\n";
}
