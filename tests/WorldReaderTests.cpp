#include "WorldReader.h"
#include <cassert>
#include <cstdint>
#include <map>
#include <iostream>

struct FakeMemory final : aoe2hd::MemoryReader {
    std::map<uint32_t, uint8_t> data;
    void put(uint32_t address, uint32_t value) {
        for (uint32_t i=0; i<4; ++i)
            data[address+i] = static_cast<uint8_t>(value >> (8*i));
    }
    bool read(uint32_t address, void* destination, size_t n) const override {
        auto* bytes = static_cast<uint8_t*>(destination);
        for (size_t i=0; i<n; ++i) {
            auto it = data.find(address+static_cast<uint32_t>(i));
            if (it == data.end()) return false;
            bytes[i] = it->second;
        }
        return true;
    }
};

int main() {
    constexpr uint32_t base=0x003B0000, owner=0x049E9F58;
    constexpr uint32_t world=0x0F5428D0, begin=0x17E24EE8;
    FakeMemory m;
    m.put(base+aoe2hd::offsets::OwnerGlobalRva,owner);
    m.put(owner+0xC8,world);
    m.put(world,base+aoe2hd::offsets::WorldVtableRva);
    m.put(world+0x174,1);
    m.put(world+0x184,begin);
    m.put(world+0x188,begin+24);
    m.put(world+0x18C,begin+24);
    const uint32_t objects[]={0x13A78734,0x1B11A85C,0x17F11854};
    const uint32_t vtables[]={aoe2hd::offsets::GaiaVtableRva,
        aoe2hd::offsets::HumanVtableRva,aoe2hd::offsets::ComputerVtableRva};
    for(uint32_t i=0;i<3;++i) {
        m.put(begin+i*8,objects[i]);
        m.put(begin+i*8+4,objects[i]-12);
        m.put(objects[i],base+vtables[i]);
        m.put(objects[i]+4,world);
        m.put(objects[i]+8,i==0?2:i==1?1:3);
    }
    auto run=[&] {return aoe2hd::ReadWorld(m,base);};
    auto valid=run();
    assert(valid.ready());
    assert(valid.snapshot.players.size()==3);
    assert(aoe2hd::GetLocalPlayer(valid)->object==objects[1]);
    m.put(begin+8+4,0);
    assert(run().status==aoe2hd::WorldStatus::InvalidPlayer);
    m.put(begin+8+4,objects[1]-12);
    m.put(begin+8,0);
    assert(run().status==aoe2hd::WorldStatus::InvalidPlayer);
    m.put(begin+8,objects[1]);
    m.put(world+0x188,begin);
    assert(run().status==aoe2hd::WorldStatus::InvalidVector);
    m.put(world+0x188,begin+24);
    m.put(world+0x174,9);
    assert(run().status==aoe2hd::WorldStatus::InvalidLocalIndex);
    m.put(world+0x174,1);
    m.put(world,0);
    assert(run().status==aoe2hd::WorldStatus::InvalidWorldVtable);
    std::cout<<"WorldReader tests passed\n";
}
