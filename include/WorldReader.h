#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace aoe2hd {

// All pointers in the target process are 32-bit, regardless of the reader's architecture.
namespace offsets {
constexpr uint32_t OwnerGlobalRva = 0x006F2C58;
constexpr uint32_t WorldVtableRva = 0x005D329C;
constexpr uint32_t GaiaVtableRva = 0x005DC8A0;
constexpr uint32_t HumanVtableRva = 0x005DCAE4;
constexpr uint32_t ComputerVtableRva = 0x005DC618;
constexpr uint32_t OwnerWorld = 0xC8;
constexpr uint32_t LocalIndex = 0x174;
constexpr uint32_t Players = 0x184;
constexpr uint32_t PlayerStride = 8;
constexpr uint32_t MaxPlayers = 9; // Gaia + up to 8 lobby players in validated samples.
}

struct MemoryReader {
    virtual ~MemoryReader() = default;
    virtual bool read(uint32_t address, void* destination, size_t bytes) const = 0;
};

struct PlayerSnapshot {
    uint32_t object = 0, controlBlock = 0, vtable = 0, worldBackPointer = 0, discriminator = 0;
};

struct WorldSnapshot {
    uint32_t world = 0;
    uint32_t localIndex = 0;
    uint32_t begin = 0, end = 0, capacityEnd = 0;
    std::vector<PlayerSnapshot> players;
};

enum class WorldStatus {
    Ready, ReadFailed, MissingOwner, MissingWorld, InvalidWorldVtable,
    InvalidVector, InvalidLocalIndex, InvalidPlayer
};

struct WorldResult {
    WorldStatus status = WorldStatus::ReadFailed;
    WorldSnapshot snapshot;
    bool ready() const { return status == WorldStatus::Ready; }
};

WorldResult ReadWorld(const MemoryReader& memory, uint32_t moduleBase);
const PlayerSnapshot* GetLocalPlayer(const WorldResult& result);

} // namespace aoe2hd
