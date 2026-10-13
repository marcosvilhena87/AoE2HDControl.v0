#pragma once
#include "ResourceReader.h"
#include <cstdint>

namespace aoe2hd {
namespace population_offsets {
constexpr uint32_t FreeSlots = 0x10;
constexpr uint32_t Used = 0x2C;
constexpr uint32_t UsedMirror = 0x4C;
constexpr uint32_t Villagers = 0x94;
}
enum class PopulationStatus {
    Ready, ResourcesNotReady, ReadFailed, ChangedDuringRead, InvalidValues, InconsistentCounters
};
struct PopulationSnapshot {
    uint32_t used = 0;
    uint32_t freeSlots = 0;
    uint32_t limit = 0;
    uint32_t villagers = 0;
};
struct PopulationResult {
    PopulationStatus status = PopulationStatus::ResourcesNotReady;
    PopulationSnapshot snapshot;
    bool ready() const { return status == PopulationStatus::Ready; }
};
// Experimental offsets validated through four controlled observations on one match.
// The ResourceResult is a freshly obtained, validated read of the same World.
PopulationResult ReadLocalPopulation(const MemoryReader& memory,
    const WorldResult& world, const ResourceResult& resources);
const char* PopulationStatusName(PopulationStatus status);
}
