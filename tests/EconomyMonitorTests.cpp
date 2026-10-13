#include "EconomyMonitor.h"
#include <cassert>
#include <iostream>
using namespace aoe2hd;
static ResourceResult ready(uint32_t player,uint32_t table,float food,float wood,float stone,float gold) {
    ResourceResult r;
    r.status=ResourceStatus::Ready;
    r.snapshot.localPlayer=player;
    r.snapshot.tableAddress=table;
    r.snapshot.food=food;r.snapshot.wood=wood;r.snapshot.stone=stone;r.snapshot.gold=gold;
    return r;
}
int main() {
    EconomyMonitor monitor;
    ResourceResult unavailable;
    assert(EconomyCsvHeader().find("run_id,elapsed_ms,session,event") == 0);
    auto absence=monitor.observe(0,0,unavailable,"ProcessNotFound");
    assert(absence.size()==1 && absence[0].type=="unavailable");
    assert(monitor.observe(0,0,unavailable,"ProcessNotFound").empty());
    auto initial=ready(0x1000,0x2000,200,200,200,100);
    auto start=monitor.observe(1,0x3000,initial,"");
    assert(start.size()==4 && start[0].session==1);
    assert(start[3].resource=="Gold" && start[3].current==100);
    assert(EconomyCsvRow(10,start[3]).find("10,1,initial,") == 0);
    assert(EconomyCsvRow(10,start[3]).find(",Gold,,100.00,,") != std::string::npos);
    assert(monitor.observe(1,0x3000,initial,"").empty());
    auto change=ready(0x1000,0x2000,150,230,200,130);
    const auto events=monitor.observe(1,0x3000,change,"");
    assert(events.size()==3);
    assert(events[0].text.find("Food: 200.00 -> 150.00 (-50.00)") != std::string::npos);
    assert(events[1].text.find("Wood: 200.00 -> 230.00 (+30.00)") != std::string::npos);
    assert(events[2].text.find("Gold: 100.00 -> 130.00 (+30.00)") != std::string::npos);
    assert(events[0].session==1 && events[0].delta==-50);
    assert(EconomyCsvRow(500,events[0]).find("500,1,change,") == 0);
    assert(monitor.observe(1,0x3001,initial,"")[0].session==2);
    assert(monitor.observe(2,0x3001,initial,"")[0].session==3);
    assert(monitor.observe(2,0x3001,ready(0x1000,0x2004,200,200,200,100),"")[0].session==4);
    assert(monitor.observe(2,0x3001,unavailable,"World:MissingWorld").size()==1);
    assert(monitor.observe(2,0x3001,unavailable,"World:MissingWorld").empty());
    assert(monitor.observe(2,0x3001,initial,"")[0].session==5);
    EconomyMonitor populationMonitor;
    assert(populationMonitor.observe(1,0x3000,initial,"").size()==4);
    PopulationResult pop;
    pop.status=PopulationStatus::Ready;
    pop.snapshot={4,1,5,3};
    auto p0=populationMonitor.observePopulation(pop);
    assert(p0.size()==4 && p0[0].type=="initial" && p0[0].session==1);
    assert(populationMonitor.observePopulation(pop).empty());
    pop.snapshot={5,0,5,4};
    auto p1=populationMonitor.observePopulation(pop);
    assert(p1.size()==3 && p1[0].resource=="PopulationUsed" && p1[0].delta==1);
    assert(EconomyCsvRow(100,p1[0]).find(",PopulationUsed,4.00,5.00,1.00,")!=std::string::npos);
    pop.snapshot={5,5,10,4};
    auto p2=populationMonitor.observePopulation(pop);
    assert(p2.size()==2 && p2[1].resource=="PopulationLimit");
    pop.snapshot={6,4,10,5};
    auto p3=populationMonitor.observePopulation(pop);
    assert(p3.size()==3);
    pop.status=PopulationStatus::InconsistentCounters;
    assert(populationMonitor.observePopulation(pop).size()==1);
    assert(populationMonitor.observePopulation(pop).empty());
    pop.status=PopulationStatus::Ready;
    assert(populationMonitor.observePopulation(pop).size()==4);
    assert(populationMonitor.observe(2,0x4000,initial,"").size()==4);
    assert(populationMonitor.observePopulation(pop)[0].session==2);
    std::cout << "EconomyMonitor CSV tests passed\n";
}
