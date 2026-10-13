#include "ResourceReader.h"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace aoe2hd {
namespace {
bool at(uint32_t base, uint32_t offset, uint32_t& address) {
    if (base > std::numeric_limits<uint32_t>::max() - offset) return false;
    address = base + offset;
    return true;
}
bool read32(const MemoryReader& memory, uint32_t base, uint32_t offset, uint32_t& out) {
    uint32_t address = 0;
    return at(base, offset, address) && memory.read(address, &out, sizeof(out));
}
}

ResourceResult ReadLocalResources(const MemoryReader& memory, const WorldResult& world) {
    ResourceResult result;
    if (!world.ready()) return result;
    const auto* local = GetLocalPlayer(world);
    if (!local || !local->object || !world.snapshot.world ||
        world.snapshot.localIndex >= world.snapshot.players.size()) {
        result.status = ResourceStatus::InvalidLocalPlayer;
        return result;
    }
    const uint32_t player = local->object;
    uint32_t backPointer = 0, table = 0, tableAgain = 0;
    if (!read32(memory, player, 4, backPointer)) {
        result.status = ResourceStatus::ReadFailed;
        return result;
    }
    if (backPointer != world.snapshot.world) {
        result.status = ResourceStatus::ChangedDuringRead;
        return result;
    }
    if (!read32(memory, player, resource_offsets::TablePointer, table)) {
        result.status = ResourceStatus::ReadFailed;
        return result;
    }
    if (!table) {
        result.status = ResourceStatus::MissingTable;
        return result;
    }
    if (table % alignof(float) != 0 ||
        table > std::numeric_limits<uint32_t>::max() - 4 * sizeof(float)) {
        result.status = ResourceStatus::InvalidValues;
        return result;
    }
    std::array<float, 4> values{};
    if (!memory.read(table, values.data(), sizeof(values))) {
        result.status = ResourceStatus::ReadFailed;
        return result;
    }
    // Guard against invalid or stale pointers without imposing arbitrary resource caps.
    for (float value : values) {
        if (!std::isfinite(value) || value < 0.0f) {
            result.status = ResourceStatus::InvalidValues;
            return result;
        }
    }
    if (!read32(memory, player, resource_offsets::TablePointer, tableAgain) ||
        !read32(memory, player, 4, backPointer)) {
        result.status = ResourceStatus::ReadFailed;
        return result;
    }
    if (tableAgain != table || backPointer != world.snapshot.world) {
        result.status = ResourceStatus::ChangedDuringRead;
        return result;
    }
    result.snapshot.localPlayer = player;
    result.snapshot.tableAddress = table;
    result.snapshot.food = values[0];
    result.snapshot.wood = values[1];
    result.snapshot.stone = values[2];
    result.snapshot.gold = values[3];
    result.status = ResourceStatus::Ready;
    return result;
}

const char* ResourceStatusName(ResourceStatus status) {
    switch (status) {
    case ResourceStatus::Ready: return "Ready";
    case ResourceStatus::WorldNotReady: return "WorldNotReady";
    case ResourceStatus::InvalidLocalPlayer: return "InvalidLocalPlayer";
    case ResourceStatus::MissingTable: return "MissingTable";
    case ResourceStatus::ReadFailed: return "ReadFailed";
    case ResourceStatus::ChangedDuringRead: return "ChangedDuringRead";
    case ResourceStatus::InvalidValues: return "InvalidValues";
    }
    return "Unknown";
}
}
