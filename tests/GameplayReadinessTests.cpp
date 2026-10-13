#include "GameplayReadinessMonitor.h"
#include <cassert>
#include <iostream>
using namespace aoe2hd;
static ResourceResult resource(float food=0) {
    ResourceResult r; r.status=ResourceStatus::Ready;
    r.snapshot.localPlayer=0x1000; r.snapshot.tableAddress=0x2000;
    r.snapshot.food=food;
    return r;
}
static PopulationResult pop(uint32_t used=0, uint32_t free=0) {
    PopulationResult p; p.status=PopulationStatus::Ready;
    p.snapshot.used=used; p.snapshot.freeSlots=free; p.snapshot.limit=used+free;
    return p;
}
int main() {
    GameplayReadinessMonitor m;
    using S=GameplayReadinessMonitor::State;
    auto zero=resource();
    auto empty=pop();
    auto initialized=resource(200);
    auto live=pop(4,1);
    assert(m.observe(1,0x3000,zero,empty)==S::Waiting);
    assert(m.observe(1,0x3000,initialized,live)==S::Waiting);
    assert(m.observe(1,0x3000,initialized,live)==S::Ready);
    // Zero balances are legitimate after gameplay becomes ready.
    assert(m.observe(1,0x3000,zero,empty)==S::Ready);
    // Transition to another player/table/world must reset readiness.
    assert(m.observe(1,0x4000,initialized,live)==S::Waiting);
    assert(m.observe(1,0x4000,initialized,live)==S::Ready);
    auto invalid=pop(); invalid.status=PopulationStatus::InconsistentCounters;
    assert(m.observe(1,0x4000,initialized,invalid)==S::Waiting);
    assert(m.observe(1,0x4000,initialized,live)==S::Waiting);
    assert(m.observe(1,0x4000,initialized,live)==S::Ready);
    assert(m.observe(2,0x4000,zero,empty)==S::Waiting);
    std::cout<<"Gameplay readiness tests passed\n";
}
