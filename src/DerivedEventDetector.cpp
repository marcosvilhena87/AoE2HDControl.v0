#include "DerivedEventDetector.h"
#include <cmath>
#include <iterator>
#include <string>
namespace aoe2hd {
void DerivedEventDetector::reset() { expenses_.clear(); session_=0; }
std::vector<EconomyEvent> DerivedEventDetector::observe(
    uint64_t elapsedMs, const std::vector<EconomyEvent>& events) {
    std::vector<EconomyEvent> output;
    for (const auto& e : events) {
        if (e.type == "initial" && e.session != session_) {
            expenses_.clear();
            session_=e.session;
        }
        if (e.type == "unavailable" || e.type == "population_unavailable") expenses_.clear();
    }
    for (const auto& e : events) {
        if (e.type == "change" && e.hasValues && e.delta < -0.005f &&
            (e.resource == "Food" || e.resource == "Wood"))
            expenses_.push_back({elapsedMs,e.session,e.resource,-e.delta});
    }
    while (!expenses_.empty() &&
        (expenses_.front().session != session_ ||
         elapsedMs < expenses_.front().time ||
         elapsedMs-expenses_.front().time > 120000))
        expenses_.pop_front();
    for (const auto& e : events) {
        if (e.type != "change" || !e.hasValues || e.delta <= 0) continue;
        const bool villager = e.resource == "VillagerCount";
        const bool capacity = e.resource == "PopulationLimit";
        if (!villager && !capacity) continue;
        EconomyEvent derived=e;
        derived.type="derived";
        derived.resource=villager ? "VillagerProduced" : "PopulationCapacityIncreased";
        derived.text=derived.resource + ": observed +" + std::to_string(static_cast<int>(e.delta));
        output.push_back(derived);
        const std::string costResource=villager ? "Food" : "Wood";
        const float cost=villager ? 50.0f : 25.0f;
        for (auto it=expenses_.rbegin();it!=expenses_.rend();++it) {
            if (it->session == e.session && it->resource == costResource &&
                std::fabs(it->amount-cost)<0.1f && elapsedMs>=it->time &&
                elapsedMs-it->time<=120000) {
                EconomyEvent hypothesis=e;
                hypothesis.type="hypothesis";
                hypothesis.resource=villager ? "PossibleVillagerFoodCost" : "PossibleHouseWoodCost";
                hypothesis.hasValues=false;
                hypothesis.text=hypothesis.resource + ": possible link to -" +
                    std::to_string(static_cast<int>(cost)) + " " + costResource +
                    " (" + std::to_string(elapsedMs-it->time) + " ms earlier)";
                output.push_back(hypothesis);
                expenses_.erase(std::next(it).base());
                break;
            }
        }
    }
    return output;
}
}
