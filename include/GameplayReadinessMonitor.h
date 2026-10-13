#pragma once
#include "PopulationReader.h"
#include <cstdint>

namespace aoe2hd {
// Operational confidence heuristic, NOT a game-engine "playing" flag.
// Two consecutive valid, coherent samples of the same player/table are required.
// During a transition, a fully zeroed snapshot is treated as an uninitialized candidate.
// Once activated, legitimate zero resources/population are permitted.
class GameplayReadinessMonitor {
public:
    enum class State { Waiting, Ready };
    State observe(uint64_t generation, uint32_t world,
                  const ResourceResult& resources, const PopulationResult& population);
    void reset();
private:
    uint64_t generation_ = 0;
    uint32_t world_ = 0, player_ = 0, table_ = 0;
    unsigned consecutive_ = 0;
    bool ready_ = false;
};
}
