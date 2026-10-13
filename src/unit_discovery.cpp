#include "ProcessMemory.h"
#include "UnitDiscovery.h"
#include "UnitContentTracker.h"
#include "UnitPointerProbe.h"
#include "ObjectFieldSampler.h"
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
}
int main(int argc,char** argv) {
    const std::string output=argc==1 ? "aoe2hd_unit_candidates.csv" :
        (argc==3 && std::string(argv[1])=="--csv" ? argv[2] : "");
    if(output.empty()){std::cerr<<"Usage: aoe2hd_unit_discovery [--csv path]\n";return 2;}
    std::ofstream csv(output,std::ios::trunc);
    if(!csv){std::cerr<<"Cannot create CSV\n";return 2;}
    csv<<"elapsed_ms,world,player,source,offset,stride,count,begin,end,capacity_end,readable_sample,reason,scanned_slots,read_failures,nonvector,empty,invalid_bounds,oversized,misaligned,unreadable\n";
    csv.flush();
    std::cout<<"Read-only discovery v6; CSV="<<output<<". Ctrl+C to stop.\n";
    aoe2hd::ProcessMemory memory;
    if(!memory.open()){std::cerr<<"AoK HD.exe not found\n";return 2;}
    std::signal(SIGINT,stop);
    const auto start=std::chrono::steady_clock::now();
    using Key=std::tuple<std::string,uint32_t,uint32_t,std::string>;
    std::map<Key,std::tuple<uint32_t,uint32_t,uint32_t>> last;
    uint32_t lastWorld=0,lastPlayer=0;
    aoe2hd::UnitContentTracker tracker;
    std::ofstream diff("aoe2hd_unit_changes.csv",std::ios::trunc);
    if(!diff) {std::cerr<<"Cannot create content diff CSV\n";return 2;}
    diff<<"elapsed_ms,world,player,source,offset,stride,index,kind,before,after,count\n";
    std::ofstream evidence("aoe2hd_unit_pointer_evidence.csv",std::ios::trunc);
    if(!evidence){std::cerr<<"Cannot create pointer evidence CSV\n";return 2;}
    evidence<<"elapsed_ms,world,player,source,offset,stride,count,sampled,nonzero,readable_targets,distinct_targets,plausible_vtables,modal_vtable,matching_vtables\n";
    aoe2hd::ObjectFieldSampler fieldSampler;
    std::ofstream fields("aoe2hd_object_fields.csv",std::ios::trunc);
    if(!fields){std::cerr<<"Cannot create object field CSV\n";return 2;}
    fields<<"elapsed_ms,world,player,offset,index,object,field_offset,kind,before,after\n";
    std::ofstream coverage("aoe2hd_object_field_summary.csv",std::ios::trunc);
    if(!coverage){std::cerr<<"Cannot create summary CSV\n";return 2;}
    coverage<<"elapsed_ms,world,player,offset,objects,fields,changes,read_failures\n";
    uint64_t lastCoverage=0;
    uint64_t lastEvidence=0;
    uint64_t lastSummary=0;
    while(!stopped) {
        auto world=aoe2hd::ReadWorld(memory,memory.moduleBase());
        const auto* player=aoe2hd::GetLocalPlayer(world);
        const auto ms=static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-start).count());
        if(!player) {
            last.clear();tracker.reset();fieldSampler.reset();lastWorld=lastPlayer=0;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        if(world.snapshot.world!=lastWorld || player->object!=lastPlayer) {
            last.clear();tracker.reset();fieldSampler.reset();lastWorld=world.snapshot.world;lastPlayer=player->object;
        }
        auto report=aoe2hd::DiagnoseUnitVectors(memory,world);
        if(lastEvidence==0 || ms-lastEvidence>=5000) {
            lastEvidence=ms ? ms : 1;
            const auto probes=aoe2hd::ProbeUnitPointers(
                memory,world,report.candidates,memory.moduleBase());
            for(const auto& e:probes) {
                evidence<<ms<<','<<world.snapshot.world<<','<<player->object<<','
                        <<e.source<<','<<e.offset<<','<<e.stride<<','<<e.count<<','
                        <<e.sampled<<','<<e.nonzero<<','<<e.readableTargets<<','
                        <<e.distinctTargets<<','<<e.plausibleVtables<<','
                        <<e.firstVtable<<','<<e.matchingVtables<<'\n';
            }
            evidence.flush();
        }
        auto fieldChanges=fieldSampler.observe(memory,report.candidates);
        for(const auto& d:fieldChanges)
            fields<<ms<<','<<world.snapshot.world<<','<<player->object<<','
                  <<d.offset<<','<<d.index<<','<<d.object<<','<<d.field<<','
                  <<d.kind<<','<<d.before<<','<<d.after<<'\n';
        if(!fieldChanges.empty())fields.flush();
        if(lastCoverage==0 || ms-lastCoverage>=5000) {
            lastCoverage=ms?ms:1;
            for(const auto& s:fieldSampler.summaries())
                coverage<<ms<<','<<world.snapshot.world<<','<<player->object<<','
                    <<s.offset<<','<<s.objects<<','<<s.fields<<','
                    <<s.changes<<','<<s.readFailures<<'\n';
            coverage.flush();
        }
        auto differences=tracker.observe(memory,report.candidates);
        for(const auto& d:differences) {
            diff<<ms<<','<<world.snapshot.world<<','<<player->object<<','
                <<d.source<<','<<d.offset<<','<<d.stride<<','<<d.index<<','
                <<d.kind<<','<<d.before<<','<<d.after<<','<<d.count<<'\n';
            std::cout<<"["<<ms<<" ms] "<<d.source<<"+0x"<<std::hex<<d.offset
                     <<std::dec<<" entry["<<d.index<<"] "<<d.kind<<" "
                     <<d.before<<" -> "<<d.after<<"\n";
        }
        if(!differences.empty())diff.flush();
        bool changed=false;
        for(const auto& c:report.candidates) {
            Key key{c.source,c.offset,c.stride,aoe2hd::UnitCandidateReasonName(c.reason)};
            auto current=std::make_tuple(c.count,c.begin,c.end);
            auto found=last.find(key);
            if(found!=last.end() && found->second==current)continue;
            last[key]=current;changed=true;
            csv<<ms<<','<<world.snapshot.world<<','<<player->object<<','
               <<c.source<<','<<c.offset<<','<<c.stride<<','<<c.count<<','
               <<c.begin<<','<<c.end<<','<<c.capacityEnd<<','<<c.readableEntries
               <<','<<aoe2hd::UnitCandidateReasonName(c.reason)
               <<','<<report.scannedSlots<<','<<report.readFailures
               <<','<<report.nonVectorTriples<<','<<report.emptyVectors
               <<','<<report.invalidBounds<<','<<report.oversized
               <<','<<report.misaligned<<','<<report.unreadable<<'\n';
            if(c.reason==aoe2hd::UnitCandidateReason::Accepted)
                std::cout<<"["<<ms<<" ms] "<<c.source<<"+0x"<<std::hex<<c.offset
                         <<std::dec<<" stride="<<c.stride<<" count="<<c.count<<"\n";
        }
        // Periodic summary ensures the CSV is informative even with zero candidates.
        if(ms-lastSummary>=5000 || lastSummary==0) {
            lastSummary=ms;
            csv<<ms<<','<<world.snapshot.world<<','<<player->object
               <<",Summary,,,,,,,,summary,"<<report.scannedSlots<<','
               <<report.readFailures<<','<<report.nonVectorTriples<<','
               <<report.emptyVectors<<','<<report.invalidBounds<<','
               <<report.oversized<<','<<report.misaligned<<','
               <<report.unreadable<<'\n';
            std::cout<<"["<<ms<<" ms] scanned="<<report.scannedSlots
                     <<" candidates="<<report.candidates.size()
                     <<" readFailures="<<report.readFailures
                     <<" nonVector="<<report.nonVectorTriples<<"\n";
        }
        if(changed || ms==lastSummary)csv.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return 0;
}
