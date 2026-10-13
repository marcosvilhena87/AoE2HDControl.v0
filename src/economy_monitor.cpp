#include "EconomyMonitor.h"
#include "ProcessMemory.h"
#include "WorldMonitor.h"
#include <chrono>
#include <csignal>
#include <cstdint>
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
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--interval-ms" && i+1 < argc &&
            parseInterval(argv[++i], intervalMs)) continue;
        std::cerr << "Usage: aoe2hd_economy_monitor [--interval-ms 250] (50..60000)\n";
        return 2;
    }
    std::signal(SIGINT, onSignal);
    aoe2hd::EconomyMonitor monitor;
    std::cout << "Read-only economy monitoring, interval=" << intervalMs
              << "ms. Ctrl+C to stop.\n";
    const auto started = std::chrono::steady_clock::now();
    // Keep the handle alive until invalidated. A process restart forces a new generation.
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
            // A terminated handle remains open but has an exit code and cannot be trusted.
            if (!memory->isAlive()) {
                memory.reset();
                failure = "ProcessNotFound";
                resources = {};
            }
        }
        const auto events = monitor.observe(generation, world.snapshot.world, resources, failure);
        for (const auto& event : events) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - started).count();
            std::cout << "[" << elapsed << " ms] " << event.text << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}
