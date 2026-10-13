#include "EconomyMonitor.h"
#include "ProcessMemory.h"
#include "WorldMonitor.h"
#include <chrono>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace {
volatile std::sig_atomic_t stopRequested = 0;
void onSignal(int) { stopRequested = 1; }
bool parseInterval(const char* value, uint32_t& out) {
    try {
        const std::string s(value);
        size_t count = 0;
        const auto n = std::stoull(s, &count);
        if (count != s.size() || n < 50 || n > 60000) return false;
        out = static_cast<uint32_t>(n);
        return true;
    } catch (...) { return false; }
}
}
int main(int argc, char** argv) {
    uint32_t intervalMs = 250;
    std::string csvPath = "aoe2hd_economy.csv";
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--interval-ms" && i+1 < argc && parseInterval(argv[++i], intervalMs)) continue;
        if (arg == "--csv" && i+1 < argc && argv[i+1][0] != '-') {
            csvPath = argv[++i];
            continue;
        }
        std::cerr << "Usage: aoe2hd_economy_monitor [--interval-ms 250] [--csv aoe2hd_economy.csv]\n";
        return 2;
    }
    // Create a new CSV for each invocation; avoids ambiguous session IDs across runs.
    std::ofstream csv(csvPath, std::ios::out | std::ios::trunc);
    if (!csv) {
        std::cerr << "Cannot create CSV: " << csvPath << "\n";
        return 2;
    }
    const auto runTimestamp = std::chrono::system_clock::now().time_since_epoch();
    const auto runId = std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(runTimestamp).count());
    csv << aoe2hd::EconomyCsvHeader() << '\n';
    csv.flush();
    if (!csv) return 2;
    std::signal(SIGINT, onSignal);
    aoe2hd::EconomyMonitor monitor;
    std::cout << "Read-only economy monitoring, interval=" << intervalMs
              << "ms, CSV=" << csvPath << ". Ctrl+C to stop.\n";
    const auto started = std::chrono::steady_clock::now();
    std::unique_ptr<aoe2hd::ProcessMemory> memory;
    uint64_t generation = 0;
    while (!stopRequested) {
        if (!memory) {
            auto candidate = std::make_unique<aoe2hd::ProcessMemory>();
            if (candidate->open()) {
                memory = std::move(candidate);
                ++generation;
            }
        }
        aoe2hd::WorldResult world;
        aoe2hd::ResourceResult resources;
        std::string failure;
        if (!memory) {
            failure = "ProcessNotFound";
        } else {
            world = aoe2hd::ReadWorld(*memory, memory->moduleBase());
            if (!world.ready()) {
                failure = std::string("World:") + aoe2hd::StatusName(world.status);
            } else {
                resources = aoe2hd::ReadLocalResources(*memory, world);
            }
            if (!memory->isAlive()) {
                memory.reset();
                failure = "ProcessNotFound";
                resources = {};
            }
        }
        const auto events = monitor.observe(generation, world.snapshot.world, resources, failure);
        const auto elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started).count());
        for (const auto& event : events) {
            // Initial rows for all four resources, one concise line on screen.
            if (event.type != "initial" || event.resource == "Food")
                std::cout << "[" << elapsed << " ms] " << event.text << '\n';
            csv << runId << ',' << aoe2hd::EconomyCsvRow(elapsed, event) << '\n';
        }
        if (!events.empty()) {
            csv.flush();
            if (!csv) {
                std::cerr << "CSV write failed: " << csvPath << "\n";
                return 3;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
    return 0;
}
