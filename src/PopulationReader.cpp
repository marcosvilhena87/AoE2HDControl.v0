#include "PopulationReader.h"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace aoe2hd {
namespace {
bool read32(const MemoryReader& memory, uint32_t address, uint32_t& value) {
    return memory.read(address, &value, sizeof(value));
}
bool readFloat(const MemoryReader& memory, uint32_t table, uint32_t offset, float& value) {
    return table <= std::numeric_limits<uint32_t>::max() - offset &&
        memory.read(table + offset, &value, sizeof(value));
}
bool counter(float value, uint32_t& result) {
    if (!std::isfinite(value) || value < 0.0f ||
        value > 100000.0f || std::fabs(value - std::round(value)) > 0.01f)
        return false;
    result = static_cast<uint32_t>(std::round(value));
    return true;
}
}
PopulationResult ReadLocalPopulation(const MemoryReader& memory,
    const WorldResult& world, const ResourceResult& resources) {
    PopulationResult result;
    if (!world.ready() || !resources.ready()) return result;
    const auto* local = GetLocalPlayer(world);
    if (!local || !local->object || local->object != resources.snapshot.localPlayer ||
        resources.snapshot.tableAddress == 0) {
        result.status = PopulationStatus::ChangedDuringRead;
        return result;
    }
    const uint32_t player = local->object;
    const uint32_t table = resources.snapshot.tableAddress;
    if (table % alignof(float) != 0 ||
        table > std::numeric_limits<uint32_t>::max() - population_offsets::Villagers) {
        result.status = PopulationStatus::InvalidValues;
        return result;
    }
    uint32_t back = 0, pointer = 0;
    if (!read32(memory, player + 4, back) ||
        !read32(memory, player + resource_offsets::TablePointer, pointer)) {
        result.status = PopulationStatus::ReadFailed;
        return result;
    }
    if (back != world.snapshot.world || pointer != table) {
        result.status = PopulationStatus::ChangedDuringRead;
        return result;
    }
    std::array<float,4> raw{};
    if (!readFloat(memory, table, population_offsets::FreeSlots, raw[0]) ||
        !readFloat(memory, table, population_offsets::Used, raw[1]) ||
        !readFloat(memory, table, population_offsets::UsedMirror, raw[2]) ||
        !readFloat(memory, table, population_offsets::Villagers, raw[3])) {
        result.status = PopulationStatus::ReadFailed;
        return result;
    }
    uint32_t free = 0, used = 0, mirror = 0, villagers = 0;
    if (!counter(raw[0], free) || !counter(raw[1], used) ||
        !counter(raw[2], mirror) || !counter(raw[3], villagers) ||
        used > std::numeric_limits<uint32_t>::max() - free) {
        result.status = PopulationStatus::InvalidValues;
        return result;
    }
    if (used != mirror || villagers > used) {
        result.status = PopulationStatus::InconsistentCounters;
        return result;
    }
    if (!read32(memory, player + resource_offsets::TablePointer, pointer) ||
        !read32(memory, player + 4, back)) {
        result.status = PopulationStatus::ReadFailed;
        return result;
    }
    if (pointer != table || back != world.snapshot.world) {
        result.status = PopulationStatus::ChangedDuringRead;
        return result;
    }
    result.snapshot = {used, free, used + free, villagers};
    result.status = PopulationStatus::Ready;
    return result;
}
const char* PopulationStatusName(PopulationStatus status) {
    switch (status) {
    case PopulationStatus::Ready: return "Ready";
    case PopulationStatus::ResourcesNotReady: return "ResourcesNotReady";
    case PopulationStatus::ReadFailed: return "ReadFailed";
    case PopulationStatus::ChangedDuringRead: return "ChangedDuringRead";
    case PopulationStatus::InvalidValues: return "InvalidValues";
    case PopulationStatus::InconsistentCounters: return "InconsistentCounters";
    }
    return "Unknown";
}
}
