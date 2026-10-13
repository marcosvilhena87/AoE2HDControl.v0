#include "ProcessMemory.h"
#include "ProductionProgressProbe.h"
#include "ResourceReader.h"
#include "PopulationReader.h"
#include "AbsoluteProgressMonitor.h"
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <tuple>
namespace {
volatile std::sig_atomic_t stopped=0;
void stop(int){stopped=1;}
std::string readPhase(const std::string& path) {
    std::ifstream f(path);
    std::string s;
    if(f && std::getline(f,s) && !s.empty() && s.size()<=32) return s;
    return "unmarked";
}
}
int main(int argc,char** argv) {
    std::string csvPath="aoe2hd_production_progress.csv";
    std::string phasePath="production_phase.txt";
    for(int i=1;i<argc;++i) {
        const std::string arg=argv[i];
        if(arg=="--csv" && i+1<argc)csvPath=argv[++i];
        else if(arg=="--phase-file" && i+1<argc)phasePath=argv[++i];
        else {std::cerr<<"Usage: aoe2hd_production_progress [--csv path] [--phase-file path]\n";return 2;}
    }
    std::ofstream csv(csvPath,std::ios::trunc);
    if(!csv){std::cerr<<"Cannot create CSV\n";return 2;}
    csv<<"elapsed_ms,phase,world,player,population_used,villagers,source,vector_offset,index,object,field_offset,encoding,raw,value,kind\n";
    csv.flush();
    aoe2hd::ProcessMemory memory;
    if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
    std::ofstream absolute("aoe2hd_absolute_progress.csv",std::ios::trunc);
    if(!absolute){std::cerr<<"Cannot create absolute progress CSV\n";return 2;}
    absolute<<"elapsed_ms,phase,world,player,population_used,villagers,address,readable16,value16,readable32,value32,kind\n";
    const std::vector<uint32_t> watched{0x142E8E54u,0x005AD5ECu,0x005AFC30u};
    aoe2hd::AbsoluteProgressMonitor absoluteMonitor;
    std::signal(SIGINT,stop);
    std::cout<<"Read-only production progress probe; CSV="<<csvPath
             <<"; phase file="<<phasePath<<". Ctrl+C to stop.\n";
    const auto start=std::chrono::steady_clock::now();
    using Key=std::tuple<uint32_t,uint32_t,uint32_t,std::string>;
    std::map<Key,std::pair<uint32_t,uint32_t>> previous; // pointer, raw
    uint32_t lastWorld=0,lastPlayer=0;
    std::string lastPhase;
    while(!stopped) {
        auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
        const auto* player=aoe2hd::GetLocalPlayer(world);
        if(!player) {
            previous.clear();absoluteMonitor.reset();lastWorld=lastPlayer=0;
            std::this_thread::sleep_for(std::chrono::milliseconds(250));continue;
        }
        if(world.snapshot.world!=lastWorld || player->object!=lastPlayer) {
            previous.clear();absoluteMonitor.reset();lastWorld=world.snapshot.world;lastPlayer=player->object;
        }
        auto resources=aoe2hd::ReadLocalResources(memory,world);
        aoe2hd::PopulationResult population;
        if(resources.ready())population=aoe2hd::ReadLocalPopulation(memory,world,resources);
        const auto report=aoe2hd::DiagnoseUnitVectors(memory,world);
        const auto rows=aoe2hd::ScanProgressCandidates(memory,report.candidates);
        const auto phase=readPhase(phasePath);
        const auto ms=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-start).count());
        const bool phaseChanged=phase!=lastPhase;
        if(phaseChanged) {
            lastPhase=phase;
            std::cout<<"["<<ms<<" ms] phase="<<phase<<" candidates="<<rows.size()<<"\n";
        }
        uint32_t used=population.ready()?population.snapshot.used:0;
        uint32_t villagers=population.ready()?population.snapshot.villagers:0;
        const auto direct=absoluteMonitor.observe(memory,watched,phaseChanged);
        for(const auto& value:direct) {
            absolute<<ms<<','<<phase<<','<<world.snapshot.world<<','<<player->object
                    <<','<<used<<','<<villagers<<','<<value.address<<','
                    <<value.readable16<<','<<value.value16<<','
                    <<value.readable32<<','<<value.value32<<','<<value.kind<<'\n';
        }
        if(!direct.empty())absolute.flush();
        unsigned emitted=0;
        for(const auto& row:rows) {
            Key key{row.vectorOffset,row.index,row.fieldOffset,row.encoding};
            auto it=previous.find(key);
            const bool initial=it==previous.end() || it->second.first!=row.object;
            const bool changed=!initial && it->second.second!=row.raw;
            // Snapshot on phase changes; changes during phases; baseline on first encounter.
            if(initial || changed || phaseChanged) {
                csv<<ms<<','<<phase<<','<<world.snapshot.world<<','<<player->object
                   <<','<<used<<','<<villagers<<','<<row.source<<','
                   <<row.vectorOffset<<','<<row.index<<','<<row.object<<','
                   <<row.fieldOffset<<','<<row.encoding<<','<<row.raw<<','
                   <<row.value<<','<<(initial?"initial":changed?"change":"phase")<<'\n';
                ++emitted;
            }
            previous[key]={row.object,row.raw};
        }
        if(emitted || phaseChanged)csv.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
    return 0;
}
