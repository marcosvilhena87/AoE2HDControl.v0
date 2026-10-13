#pragma once
#include "WorldReader.h"
#include <cstdint>

namespace aoe2hd {
namespace resource_offsets {
constexpr uint32_t TablePointer = 0x3C;
constexpr uint32_t Food = 0x00;
constexpr uint32_t Wood = 0x04;
constexpr uint32_t Stone = 0x08;
constexpr uint32_t Gold = 0x0C;
}

struct ResourceSnapshot {
    uint32_t localPlayer = 0;
    uint32_t tableAddress = 0;
    float food = 0.0f;
    float wood = 0.0f;
    float stone = 0.0f;
    float gold = 0.0f;
};

enum class ResourceStatus {
    Ready,
    WorldNotReady,
    InvalidLocalPlayer,
    MissingTable,
    ReadFailed,
    ChangedDuringRead,
    InvalidValues
};

struct ResourceResult {
    ResourceStatus status = ResourceStatus::WorldNotReady;
    ResourceSnapshot snapshot;
    bool ready() const { return status == ResourceStatus::Ready; }
};

// Consumes a validated World snapshot; detects common player/table changes.
// The result is a detached snapshot, not an atomic game-state transaction.
ResourceResult ReadLocalResources(const MemoryReader& memory, const WorldResult& world);
const char* ResourceStatusName(ResourceStatus status);
}
