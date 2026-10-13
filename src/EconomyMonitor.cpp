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
    return "run_id,elapsed_ms,session,event,world,local_player,resource_table,resource,previous,current,delta,details";
}
std::string EconomyCsvRow(uint64_t elapsedMs, const EconomyEvent& e) {
    std::ostringstream s;
    s.imbue(std::locale::classic());
    s << elapsedMs << ',' << e.session << ',' << e.type << ','
      << e.world << ',' << e.player << ',' << e.table << ','
      << e.resource << ',';
    if (e.hasValues && e.type == "initial") s << ',' << fixed(e.current) << ',';
    else if (e.hasValues) s << fixed(e.previous) << ',' << fixed(e.current) << ',' << fixed(e.delta);
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
        populationActive_ = false;
        failure_ = reason;
        return events;
    }
    failure_.clear();
    const auto& snap = resources.snapshot;
    const auto now = values(snap);
    const bool newSession = !active_ || generation != generation_ || world != world_ ||
        snap.localPlayer != player_ || snap.tableAddress != table_;
    if (newSession) {
        populationActive_ = false;
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
std::vector<EconomyEvent> EconomyMonitor::observePopulation(const PopulationResult& population) {
    std::vector<EconomyEvent> events;
    if (!active_) return events;
    if (!population.ready()) {
        const std::string reason = PopulationStatusName(population.status);
        if (populationActive_ || reason != populationFailure_) {
            EconomyEvent e;
            e.type = "population_unavailable";
            e.session = session_;
            e.world = world_;
            e.player = player_;
            e.table = table_;
            e.text = "PopulationUnavailable: " + reason;
            events.push_back(e);
        }
        populationActive_ = false;
        populationFailure_ = reason;
        return events;
    }
    populationFailure_.clear();
    const auto& p = population.snapshot;
    const std::array<uint32_t,4> current{{p.used,p.freeSlots,p.limit,p.villagers}};
    const std::array<const char*,4> names{{"PopulationUsed","FreePopulationSlots","PopulationLimit","VillagerCount"}};
    for (size_t i = 0; i < current.size(); ++i) {
        if (populationActive_ && previousPopulation_[i] == current[i]) continue;
        EconomyEvent e;
        e.session = session_;
        e.world = world_;
        e.player = player_;
        e.table = table_;
        e.resource = names[i];
        e.type = populationActive_ ? "change" : "initial";
        e.hasValues = true;
        e.current = static_cast<float>(current[i]);
        if (populationActive_) {
            e.previous = static_cast<float>(previousPopulation_[i]);
            e.delta = e.current - e.previous;
            e.text = e.resource + ": " + std::to_string(previousPopulation_[i]) +
                " -> " + std::to_string(current[i]) +
                " (" + (e.delta >= 0 ? "+" : "") + std::to_string(static_cast<int>(e.delta)) + ")";
        } else {
            e.text = "Initial " + e.resource + "=" + std::to_string(current[i]);
        }
        events.push_back(e);
    }
    previousPopulation_ = current;
    populationActive_ = true;
    return events;
}

}
