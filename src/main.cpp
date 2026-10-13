#include "ProcessMemory.h"
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
    return 0;
}
