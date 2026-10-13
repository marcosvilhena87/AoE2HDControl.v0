#include "EconomyMonitor.h"
#include <array>
#include <iomanip>
#include <sstream>
#include <utility>

namespace aoe2hd {
namespace {
std::array<float, 4> values(const ResourceSnapshot& r) {
    return {r.food, r.wood, r.stone, r.gold};
}
std::string fixed(float v) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(2) << v;
    return s.str();
}
}
std::vector<EconomyEvent> EconomyMonitor::observe(uint64_t generation, uint32_t world,
    const ResourceResult& resources, const std::string& failure) {
    std::vector<EconomyEvent> events;
    if (!resources.ready()) {
        const std::string reason = failure.empty() ? ResourceStatusName(resources.status) : failure;
        if (active_ || failure_ != reason) events.push_back({"Unavailable: " + reason});
        active_ = false;
        failure_ = reason;
        return events;
    }
    failure_.clear();
    const auto& snap = resources.snapshot;
    const auto now = values(snap);
    if (!active_ || generation != generation_ || world != world_ ||
        snap.localPlayer != player_ || snap.tableAddress != table_) {
        std::ostringstream s;
        s << "ResourcesReady World=0x" << std::hex << world
          << " Player=0x" << snap.localPlayer << " Table=0x" << snap.tableAddress
          << std::dec << " Food=" << fixed(now[0]) << " Wood=" << fixed(now[1])
          << " Stone=" << fixed(now[2]) << " Gold=" << fixed(now[3]);
        events.push_back({s.str()});
    } else {
        constexpr std::array<const char*, 4> names{{"Food", "Wood", "Stone", "Gold"}};
        for (size_t i = 0; i < now.size(); ++i) {
            // Ignore insignificant float noise; never update the baseline for ignored changes.
            if (now[i] > previous_[i] + 0.005f || now[i] < previous_[i] - 0.005f) {
                const auto delta = now[i] - previous_[i];
                events.push_back({std::string(names[i]) + ": " + fixed(previous_[i]) +
                    " -> " + fixed(now[i]) + " (" + (delta >= 0 ? "+" : "") +
                    fixed(delta) + ")"});
            }
        }
    }
    active_ = true;
    generation_ = generation;
    world_ = world;
    player_ = snap.localPlayer;
    table_ = snap.tableAddress;
    previous_ = now;
    return events;
}
}
