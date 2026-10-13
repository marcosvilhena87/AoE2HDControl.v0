#include "ProcessMemory.h"
#include "UnitDiscovery.h"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <tuple>
int main(int argc,char** argv) {
    const std::string output=argc==1 ? "aoe2hd_unit_candidates.csv" :
        (argc==3 && std::string(argv[1])=="--csv" ? argv[2] : "");
    if (output.empty()) {
        std::cerr<<"Usage: aoe2hd_unit_discovery [--csv path]\n";
        return 2;
    }
    std::ofstream csv(output,std::ios::trunc);
    if (!csv) {std::cerr<<"Cannot create CSV\n";return 2;}
    csv<<"elapsed_ms,world,player,offset,stride,count,begin,end,capacity_end,readable_sample\n";
    std::cout<<"Read-only unit candidate discovery; CSV="<<output
             <<". Start with 3 villagers; train one, then Ctrl+C.\n";
    aoe2hd::ProcessMemory memory;
    if (!memory.open()) {std::cerr<<"AoK HD.exe not found\n";return 2;}
    const auto start=std::chrono::steady_clock::now();
    std::map<std::pair<uint32_t,uint32_t>,uint32_t> last;
    uint32_t lastWorld=0,lastPlayer=0;
    // Fixed-duration diagnostic capture; Ctrl+C terminates Windows console process.
    while (true) {
        auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
        const auto* player=aoe2hd::GetLocalPlayer(world);
        if (!world.ready() || !player) {
            last.clear();lastWorld=lastPlayer=0;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        if (world.snapshot.world!=lastWorld || player->object!=lastPlayer) {
            last.clear();lastWorld=world.snapshot.world;lastPlayer=player->object;
        }
        auto candidates=aoe2hd::DiscoverUnitVectors(memory,world);
        const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-start).count();
        for(const auto& c:candidates) {
            const auto key=std::make_pair(c.offset,c.stride);
            auto found=last.find(key);
            if (found!=last.end() && found->second==c.count) continue;
            last[key]=c.count;
            csv<<ms<<','<<world.snapshot.world<<','<<player->object<<','
               <<c.offset<<','<<c.stride<<','<<c.count<<','<<c.begin<<','
               <<c.end<<','<<c.capacityEnd<<','<<c.readableEntries<<'\n';
            std::cout<<"["<<ms<<" ms] candidate player+0x"<<std::hex<<c.offset
                     <<std::dec<<" stride="<<c.stride<<" count="<<c.count<<"\n";
        }
        csv.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
