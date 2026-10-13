#include "GameplayReadinessMonitor.h"

namespace aoe2hd {
void GameplayReadinessMonitor::reset() {
    generation_ = 0;
    world_ = player_ = table_ = 0;
    consecutive_ = 0;
    ready_ = false;
}
GameplayReadinessMonitor::State GameplayReadinessMonitor::observe(
    uint64_t generation, uint32_t world,
    const ResourceResult& resources, const PopulationResult& population) {
    if (!resources.ready() || !population.ready() || !world) {
        reset();
        return State::Waiting;
    }
    const auto& r = resources.snapshot;
    const auto& p = population.snapshot;
    if (generation != generation_ || world != world_ ||
        r.localPlayer != player_ || r.tableAddress != table_) {
        generation_ = generation;
        world_ = world;
        player_ = r.localPlayer;
        table_ = r.tableAddress;
        consecutive_ = 0;
        ready_ = false;
    }
    if (ready_) return State::Ready;
    // Empirically, fresh World snapshots were all-zero for one poll.
    // Nonzero fields are evidence of initialization, but not proof of active gameplay.
    const bool allZero = r.food == 0 && r.wood == 0 &&
        r.stone == 0 && r.gold == 0 &&
        p.used == 0 && p.freeSlots == 0 && p.limit == 0 && p.villagers == 0;
    if (allZero) {
        consecutive_ = 0;
        return State::Waiting;
    }
    if (++consecutive_ >= 2) ready_ = true;
    return ready_ ? State::Ready : State::Waiting;
}
}
