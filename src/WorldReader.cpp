#include "WorldReader.h"
#include <array>
#include <cstdint>
#include <limits>
#include <utility>

namespace aoe2hd {
namespace {
bool add(uint32_t a, uint32_t b, uint32_t& value) {
    if (b > std::numeric_limits<uint32_t>::max() - a) return false;
    value = a + b;
    return true;
}
bool get32(const MemoryReader& memory, uint32_t base, uint32_t delta, uint32_t& value) {
    uint32_t address = 0;
    return add(base, delta, address) && memory.read(address, &value, sizeof(value));
}
bool expected(uint32_t moduleBase, uint32_t rva, uint32_t& value) {
    return add(moduleBase, rva, value);
}
}

WorldResult ReadWorld(const MemoryReader& memory, uint32_t moduleBase) {
    WorldResult out;
    uint32_t ownerGlobal = 0, owner = 0, world = 0, vtable = 0, wanted = 0;
    if (!expected(moduleBase, offsets::OwnerGlobalRva, ownerGlobal) ||
        !memory.read(ownerGlobal, &owner, 4)) return out;
    if (!owner) { out.status = WorldStatus::MissingOwner; return out; }
    if (!get32(memory, owner, offsets::OwnerWorld, world)) return out;
    if (!world) { out.status = WorldStatus::MissingWorld; return out; }
    if (!get32(memory, world, 0, vtable) ||
        !expected(moduleBase, offsets::WorldVtableRva, wanted)) return out;
    if (vtable != wanted) { out.status = WorldStatus::InvalidWorldVtable; return out; }

    uint32_t local = 0, begin = 0, end = 0, cap = 0;
    if (!get32(memory, world, offsets::LocalIndex, local) ||
        !get32(memory, world, offsets::Players, begin) ||
        !get32(memory, world, offsets::Players + 4, end) ||
        !get32(memory, world, offsets::Players + 8, cap)) return out;

    // Avoid unsigned subtraction until all bounds are known consistent.
    if (!begin || (begin % 4) || end < begin || cap < end ||
        ((end - begin) % offsets::PlayerStride) ||
        ((cap - begin) % offsets::PlayerStride) ||
        (end - begin) / offsets::PlayerStride < 2 ||
        (cap - begin) / offsets::PlayerStride > offsets::MaxPlayers) {
        out.status = WorldStatus::InvalidVector;
        return out;
    }
    const uint32_t count = (end - begin) / offsets::PlayerStride;
    if (local == 0 || local >= count) {
        out.status = WorldStatus::InvalidLocalIndex;
        return out;
    }

    WorldSnapshot snap;
    snap.world = world;
    snap.localIndex = local;
    snap.begin = begin;
    snap.end = end;
    snap.capacityEnd = cap;
    snap.players.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        PlayerSnapshot p;
        uint32_t entry = 0;
        if (!add(begin, i * offsets::PlayerStride, entry) ||
            !get32(memory, entry, 0, p.object) ||
            !get32(memory, entry, 4, p.controlBlock)) return out;
        if (!p.object || !p.controlBlock || p.controlBlock > UINT32_MAX - 12 ||
            p.object != p.controlBlock + 12 ||
            !get32(memory, p.object, 0, p.vtable) ||
            !get32(memory, p.object, 4, p.worldBackPointer) ||
            !get32(memory, p.object, 8, p.discriminator)) {
            out.status = WorldStatus::InvalidPlayer;
            return out;
        }
        uint32_t expectedVtable = 0;
        const uint32_t rva = i == 0 ? offsets::GaiaVtableRva :
            (i == local ? offsets::HumanVtableRva : offsets::ComputerVtableRva);
        if (!expected(moduleBase, rva, expectedVtable) ||
            p.vtable != expectedVtable || p.worldBackPointer != world ||
            p.discriminator != (i == 0 ? 2u : i == local ? 1u : 3u)) {
            out.status = WorldStatus::InvalidPlayer;
            return out;
        }
        snap.players.push_back(p);
    }
    // Recheck the anchors to reject a torn snapshot from a changing game state.
    uint32_t ownerAgain = 0, worldAgain = 0, beginAgain = 0, endAgain = 0, capAgain = 0, localAgain = 0;
    if (!memory.read(ownerGlobal, &ownerAgain, 4) ||
        !get32(memory, owner, offsets::OwnerWorld, worldAgain) ||
        !get32(memory, world, offsets::Players, beginAgain) ||
        !get32(memory, world, offsets::Players + 4, endAgain) ||
        !get32(memory, world, offsets::Players + 8, capAgain) ||
        !get32(memory, world, offsets::LocalIndex, localAgain)) return out;
    if (ownerAgain != owner || worldAgain != world || beginAgain != begin ||
        endAgain != end || capAgain != cap || localAgain != local) {
        out.status = WorldStatus::InvalidVector;
        return out;
    }
    out.status = WorldStatus::Ready;
    out.snapshot = std::move(snap);
    return out;
}

const PlayerSnapshot* GetLocalPlayer(const WorldResult& result) {
    if (!result.ready() || result.snapshot.localIndex >= result.snapshot.players.size())
        return nullptr;
    return &result.snapshot.players[result.snapshot.localIndex];
}
} // namespace aoe2hd
