#include "EconomyMonitor.h"
#include <cassert>
#include <iostream>
using namespace aoe2hd;
static ResourceResult ready(uint32_t player, uint32_t table, float food, float wood, float stone, float gold) {
    ResourceResult r;
    r.status = ResourceStatus::Ready;
    r.snapshot.localPlayer = player;
    r.snapshot.tableAddress = table;
    r.snapshot.food = food;
    r.snapshot.wood = wood;
    r.snapshot.stone = stone;
    r.snapshot.gold = gold;
    return r;
}
int main() {
    EconomyMonitor monitor;
    ResourceResult unavailable;
    assert(monitor.observe(0, 0, unavailable, "ProcessNotFound").size() == 1);
    assert(monitor.observe(0, 0, unavailable, "ProcessNotFound").empty());
    auto initial = ready(0x1000, 0x2000, 200, 200, 200, 100);
    assert(monitor.observe(1, 0x3000, initial, "").size() == 1);
    assert(monitor.observe(1, 0x3000, initial, "").empty());
    auto change = ready(0x1000, 0x2000, 150, 230, 200, 130);
    const auto events = monitor.observe(1, 0x3000, change, "");
    assert(events.size() == 3);
    assert(events[0].text.find("Food: 200.00 -> 150.00 (-50.00)") != std::string::npos);
    assert(events[1].text.find("Wood: 200.00 -> 230.00 (+30.00)") != std::string::npos);
    assert(events[2].text.find("Gold: 100.00 -> 130.00 (+30.00)") != std::string::npos);
    assert(monitor.observe(1, 0x3001, initial, "").size() == 1);
    assert(monitor.observe(2, 0x3001, initial, "").size() == 1);
    assert(monitor.observe(2, 0x3001, ready(0x1000, 0x2004, 200,200,200,100), "").size() == 1);
    assert(monitor.observe(2, 0x3001, unavailable, "World:MissingWorld").size() == 1);
    assert(monitor.observe(2, 0x3001, initial, "").size() == 1);
    std::cout << "EconomyMonitor tests passed\n";
}
