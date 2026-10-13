#pragma once
#include "ResourceReader.h"
#include "PopulationReader.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace aoe2hd {
struct EconomyEvent {
    std::string text;
    std::string type; // initial, change, unavailable
    uint64_t session = 0;
    uint32_t world = 0;
    uint32_t player = 0;
    uint32_t table = 0;
    std::string resource;
    float previous = 0, current = 0, delta = 0;
    bool hasValues = false;
};
std::string EconomyCsvHeader();
std::string EconomyCsvRow(uint64_t elapsedMs, const EconomyEvent& event);

class EconomyMonitor {
public:
    std::vector<EconomyEvent> observe(uint64_t generation, uint32_t world,
        const ResourceResult& resources, const std::string& failure);
    std::vector<EconomyEvent> observePopulation(const PopulationResult& population);
private:
    bool active_ = false;
    uint64_t generation_ = 0;
    uint64_t session_ = 0;
    uint32_t world_ = 0, player_ = 0, table_ = 0;
    std::array<float, 4> previous_{};
    std::string failure_;
    bool populationActive_ = false;
    std::array<uint32_t, 4> previousPopulation_{};
    std::string populationFailure_;
};
}
