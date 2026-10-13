#include "DerivedEventDetector.h"
#include <cassert>
#include <iostream>
using namespace aoe2hd;
static EconomyEvent change(uint64_t session,const char* resource,float delta) {
    EconomyEvent e; e.type="change"; e.session=session; e.resource=resource;
    e.delta=delta;e.hasValues=true;e.current=delta;
    return e;
}
int main(){
    DerivedEventDetector d;
    auto initial=change(1,"Food",0);initial.type="initial";
    assert(d.observe(0,{initial}).empty());
    assert(d.observe(1000,{change(1,"Food",-50)}).empty());
    auto found=d.observe(3000,{change(1,"VillagerCount",1)});
    assert(found.size()==2 && found[0].resource=="VillagerProduced");
    assert(found[1].type=="hypothesis");
    assert(d.observe(4000,{change(1,"VillagerCount",1)}).size()==1);
    d.observe(5000,{change(1,"Wood",-25)});
    auto house=d.observe(6000,{change(1,"PopulationLimit",5)});
    assert(house.size()==2 && house[0].resource=="PopulationCapacityIncreased");
    auto next=initial;next.session=2;
    assert(d.observe(7000,{next}).empty());
    assert(d.observe(8000,{change(2,"VillagerCount",1)}).size()==1);
    d.observe(9000,{change(2,"Food",-50)});
    auto outage=initial;outage.type="unavailable";outage.session=2;
    d.observe(9500,{outage});
    assert(d.observe(10000,{change(2,"VillagerCount",1)}).size()==1);
    d.observe(11000,{change(2,"Food",-50)});
    assert(d.observe(132000,{change(2,"VillagerCount",1)}).size()==1);
    std::cout<<"Derived event detector tests passed\n";
}
