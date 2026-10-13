#include "ResourceReader.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>

struct FakeMemory final : aoe2hd::MemoryReader {
    std::map<uint32_t, uint8_t> bytes;
    void put32(uint32_t address, uint32_t value) {
        for (uint32_t i = 0; i < 4; ++i)
            bytes[address + i] = static_cast<uint8_t>(value >> (8 * i));
    }
    void putFloat(uint32_t address, float value) {
        uint32_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value), "32-bit float required");
        std::memcpy(&bits, &value, sizeof(bits));
        put32(address, bits);
    }
    bool read(uint32_t address, void* target, size_t size) const override {
        auto* out = static_cast<uint8_t*>(target);
        for (size_t i = 0; i < size; ++i) {
            auto it = bytes.find(address + static_cast<uint32_t>(i));
            if (it == bytes.end()) return false;
            out[i] = it->second;
        }
        return true;
    }
};

int main() {
    using namespace aoe2hd;
    constexpr uint32_t player = 0x14D7A01C;
    constexpr uint32_t worldAddress = 0x145EECA8;
    constexpr uint32_t table = 0x145C1038;
    FakeMemory m;
    m.put32(player + 4, worldAddress);
    m.put32(player + resource_offsets::TablePointer, table);
    m.putFloat(table + resource_offsets::Food, 260.145f);
    m.putFloat(table + resource_offsets::Wood, 230.0f);
    m.putFloat(table + resource_offsets::Stone, 230.0f);
    m.putFloat(table + resource_offsets::Gold, 160.0f);

    WorldResult world;
    assert(ReadLocalResources(m, world).status == ResourceStatus::WorldNotReady);
    world.status = WorldStatus::Ready;
    world.snapshot.world = worldAddress;
    world.snapshot.localIndex = 1;
    world.snapshot.players.resize(3);
    world.snapshot.players[1].object = player;

    auto result = ReadLocalResources(m, world);
    assert(result.ready());
    assert(result.snapshot.tableAddress == table);
    assert(std::fabs(result.snapshot.food - 260.145f) < 0.001f);
    assert(result.snapshot.wood == 230.0f);
    assert(result.snapshot.stone == 230.0f);
    assert(result.snapshot.gold == 160.0f);

    m.putFloat(table + resource_offsets::Gold, 190.0f);
    assert(ReadLocalResources(m, world).snapshot.gold == 190.0f);
    m.put32(player + resource_offsets::TablePointer, 0);
    assert(ReadLocalResources(m, world).status == ResourceStatus::MissingTable);
    m.put32(player + resource_offsets::TablePointer, table);
    m.bytes.erase(table);
    assert(ReadLocalResources(m, world).status == ResourceStatus::ReadFailed);
    m.putFloat(table, -1.0f);
    assert(ReadLocalResources(m, world).status == ResourceStatus::InvalidValues);
    m.putFloat(table, 260.145f);
    m.put32(player + 4, 0);
    assert(ReadLocalResources(m, world).status == ResourceStatus::ChangedDuringRead);
    std::cout << "ResourceReader tests passed\n";
}
