#include "EconomyMonitor.h"
#include <array>
#include <iomanip>
#include <locale>
#include <sstream>

namespace aoe2hd {
namespace {
std::array<float,4> values(const ResourceSnapshot& r) { return {r.food,r.wood,r.stone,r.gold}; }
constexpr std::array<const char*,4> names{{"Food","Wood","Stone","Gold"}};
std::string fixed(float v) {
    std::ostringstream s;
    s.imbue(std::locale::classic());
    s << std::fixed << std::setprecision(2) << v;
    return s.str();
}
std::string escaped(const std::string& s) {
    std::string result = "\"";
    for (char c : s) {
        if (c == '"') result += "\"\"";
        else if (c == '\r' || c == '\n') result += ' ';
        else result += c;
    }
    return result + "\"";
}
}
std::string EconomyCsvHeader() {
    return "elapsed_ms,session,event,world,local_player,resource_table,resource,previous,current,delta,details";
}
std::string EconomyCsvRow(uint64_t elapsedMs, const EconomyEvent& e) {
    std::ostringstream s;
    s.imbue(std::locale::classic());
    s << elapsedMs << ',' << e.session << ',' << e.type << ','
      << e.world << ',' << e.player << ',' << e.table << ','
      << e.resource << ',';
    if (e.hasValues) s << fixed(e.previous) << ',' << fixed(e.current) << ',' << fixed(e.delta);
    else s << ",,";
    s << ',' << escaped(e.text);
    return s.str();
}
std::vector<EconomyEvent> EconomyMonitor::observe(uint64_t generation, uint32_t world,
    const ResourceResult& resources, const std::string& failure) {
    std::vector<EconomyEvent> events;
    if (!resources.ready()) {
        const std::string reason = failure.empty() ? ResourceStatusName(resources.status) : failure;
        if (active_ || failure_ != reason) {
            EconomyEvent e;
            e.text = "Unavailable: " + reason;
            e.type = "unavailable";
            e.session = session_;
            e.world = world_;
            e.player = player_;
            e.table = table_;
            events.push_back(e);
        }
        active_ = false;
        failure_ = reason;
        return events;
    }
    failure_.clear();
    const auto& snap = resources.snapshot;
    const auto now = values(snap);
    const bool newSession = !active_ || generation != generation_ || world != world_ ||
        snap.localPlayer != player_ || snap.tableAddress != table_;
    if (newSession) {
        ++session_;
        std::ostringstream s;
        s << "ResourcesReady World=0x" << std::hex << world
          << " Player=0x" << snap.localPlayer << " Table=0x" << snap.tableAddress
          << std::dec << " Food=" << fixed(now[0]) << " Wood=" << fixed(now[1])
          << " Stone=" << fixed(now[2]) << " Gold=" << fixed(now[3]);
        for (size_t i = 0; i < now.size(); ++i) {
            EconomyEvent e;
            e.text = i == 0 ? s.str() : std::string("Initial ") + names[i] + "=" + fixed(now[i]);
            e.type = "initial";
            e.session = session_;
            e.world = world;
            e.player = snap.localPlayer;
            e.table = snap.tableAddress;
            e.resource = names[i];
            e.current = now[i];
            e.hasValues = true;
            events.push_back(e);
        }
    } else {
        for (size_t i = 0; i < now.size(); ++i) {
            if (now[i] > previous_[i] + 0.005f || now[i] < previous_[i] - 0.005f) {
                const float delta = now[i] - previous_[i];
                EconomyEvent e;
                e.text = std::string(names[i]) + ": " + fixed(previous_[i]) +
                    " -> " + fixed(now[i]) + " (" + (delta >= 0 ? "+" : "") + fixed(delta) + ")";
                e.type = "change";
                e.session = session_;
                e.world = world;
                e.player = snap.localPlayer;
                e.table = snap.tableAddress;
                e.resource = names[i];
                e.previous = previous_[i];
                e.current = now[i];
                e.delta = delta;
                e.hasValues = true;
                events.push_back(e);
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
