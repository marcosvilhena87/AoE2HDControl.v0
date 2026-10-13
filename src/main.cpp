#include "ProcessMemory.h"
#include "ResourceReader.h"
#include "PopulationReader.h"
#include <iomanip>
#include <iostream>

int main() {
    aoe2hd::ProcessMemory memory;
    if (!memory.open()) {
        std::cerr << "Unable to open AoK HD.exe (read-only). Check process and bitness.\n";
        return 2;
    }
    const auto result = aoe2hd::ReadWorld(memory, memory.moduleBase());
    std::cout << "WorldStructurallyReady=" << (result.ready() ? "true" : "false")
              << " status=" << static_cast<int>(result.status) << "\n";
    if (!result.ready()) return 1;
    const auto& s = result.snapshot;
    std::cout << std::hex << "World=0x" << s.world << std::dec
              << " localIndex=" << s.localIndex
              << " players=" << s.players.size() << "\n";
    for (size_t i = 0; i < s.players.size(); ++i) {
        std::cout << i << " object=0x" << std::hex << s.players[i].object
                  << " vtable=0x" << s.players[i].vtable << std::dec << "\n";
    }
    const auto resources = aoe2hd::ReadLocalResources(memory, result);
    std::cout << "ResourcesStatus=" << aoe2hd::ResourceStatusName(resources.status) << "\n";
    if (!resources.ready()) return 3;
    const auto& r = resources.snapshot;
    std::cout << "LocalPlayer=0x" << std::hex << r.localPlayer
              << " ResourceTable=0x" << r.tableAddress << std::dec << "\n";
    std::cout << std::fixed << std::setprecision(2)
              << "Food=" << r.food << " Wood=" << r.wood
              << " Stone=" << r.stone << " Gold=" << r.gold << "\n";
    const auto population = aoe2hd::ReadLocalPopulation(memory, result, resources);
    std::cout << "PopulationStatus=" << aoe2hd::PopulationStatusName(population.status) << "\n";
    if (!population.ready()) return 4;
    const auto& p = population.snapshot;
    std::cout << "CurrentPopulation=" << p.used
              << " FreePopulationSlots=" << p.freeSlots
              << " PopulationLimit=" << p.limit
              << " VillagerCount=" << p.villagers << "\n";
    return 0;
}
