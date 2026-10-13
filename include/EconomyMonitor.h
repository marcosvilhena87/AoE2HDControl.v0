#pragma once
#include "ResourceReader.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace aoe2hd {
struct EconomyEvent {
    std::string text;
};

// Samples must come from a fresh ReadWorld / ReadLocalResources pair.
// The processId-like generation key should change when the target process restarts.
class EconomyMonitor {
public:
    std::vector<EconomyEvent> observe(uint64_t generation, uint32_t world,
        const ResourceResult& resources, const std::string& failure);
private:
    bool active_ = false;
    uint64_t generation_ = 0;
    uint32_t world_ = 0;
    uint32_t player_ = 0;
    uint32_t table_ = 0;
    std::array<float, 4> previous_{};
    std::string failure_;
};
}
