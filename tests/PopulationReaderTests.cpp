#include "PopulationReader.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>

struct FakeMemory final : aoe2hd::MemoryReader {
    std::map<uint32_t,uint8_t> bytes;
    void put32(uint32_t a,uint32_t n) {
        for(uint32_t i=0;i<4;++i) bytes[a+i]=static_cast<uint8_t>(n>>(8*i));
    }
    void putFloat(uint32_t a,float f) {
        uint32_t n=0; std::memcpy(&n,&f,4); put32(a,n);
    }
    bool read(uint32_t a,void* dest,size_t n) const override {
        auto* p=static_cast<uint8_t*>(dest);
        for(size_t i=0;i<n;++i) {
            auto it=bytes.find(a+static_cast<uint32_t>(i));
            if(it==bytes.end()) return false;
            p[i]=it->second;
        }
        return true;
    }
};
int main() {
    using namespace aoe2hd;
    constexpr uint32_t player=0x13BD055C, worldAddress=0x140BBCE8, table=0x188F1140;
    FakeMemory mem;
    mem.put32(player+4, worldAddress);
    mem.put32(player+resource_offsets::TablePointer,table);
    WorldResult world;
    ResourceResult resources;
    assert(ReadLocalPopulation(mem,world,resources).status==PopulationStatus::ResourcesNotReady);
    world.status=WorldStatus::Ready;
    world.snapshot.world=worldAddress;
    world.snapshot.localIndex=1;
    world.snapshot.players.resize(3);
    world.snapshot.players[1].object=player;
    resources.status=ResourceStatus::Ready;
    resources.snapshot.localPlayer=player;
    resources.snapshot.tableAddress=table;
    auto state=[&](float free,float used,float mirror,float villagers) {
        mem.putFloat(table+population_offsets::FreeSlots,free);
        mem.putFloat(table+population_offsets::Used,used);
        mem.putFloat(table+population_offsets::UsedMirror,mirror);
        mem.putFloat(table+population_offsets::Villagers,villagers);
    };
    state(1,4,4,3);
    auto a=ReadLocalPopulation(mem,world,resources);
    assert(a.ready() && a.snapshot.used==4 && a.snapshot.limit==5 && a.snapshot.villagers==3);
    state(0,5,5,4);
    auto b=ReadLocalPopulation(mem,world,resources);
    assert(b.ready() && b.snapshot.freeSlots==0 && b.snapshot.limit==5);
    state(5,5,5,4);
    auto c=ReadLocalPopulation(mem,world,resources);
    assert(c.ready() && c.snapshot.limit==10);
    state(4,6,6,5);
    auto d=ReadLocalPopulation(mem,world,resources);
    assert(d.ready() && d.snapshot.used==6 && d.snapshot.limit==10 && d.snapshot.villagers==5);
    state(4,6,5,5);
    assert(ReadLocalPopulation(mem,world,resources).status==PopulationStatus::InconsistentCounters);
    state(4,6,6,7);
    assert(ReadLocalPopulation(mem,world,resources).status==PopulationStatus::InconsistentCounters);
    state(-1,6,6,5);
    assert(ReadLocalPopulation(mem,world,resources).status==PopulationStatus::InvalidValues);
    state(4,6,6,5);
    mem.bytes.erase(table+population_offsets::Used);
    assert(ReadLocalPopulation(mem,world,resources).status==PopulationStatus::ReadFailed);
    state(4,6,6,5);
    mem.put32(player+4,0);
    assert(ReadLocalPopulation(mem,world,resources).status==PopulationStatus::ChangedDuringRead);
    std::cout<<"PopulationReader tests passed\n";
}
