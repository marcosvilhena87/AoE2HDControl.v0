#pragma once
#include "EconomyMonitor.h"
#include <cstdint>
#include <deque>
#include <vector>
namespace aoe2hd {
class DerivedEventDetector {
public:
    std::vector<EconomyEvent> observe(uint64_t elapsedMs, const std::vector<EconomyEvent>& events);
    void reset();
private:
    struct Expenditure {uint64_t time; uint64_t session; std::string resource; float amount;};
    std::deque<Expenditure> expenses_;
    uint64_t session_ = 0;
};
}