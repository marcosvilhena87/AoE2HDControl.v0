#include "ProcessMemory.h"
#include "WorldMonitor.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace {
volatile std::sig_atomic_t stopped = 0;
void onSignal(int) { stopped = 1; }
bool parsePositive(const char* input, uint32_t& value) {
    try {
        size_t parsed = 0;
        const auto n = std::stoull(input, &parsed, 10);
        if (parsed != std::string(input).size() || n == 0 || n > 60000) return false;
        value = static_cast<uint32_t>(n);
        return true;
    } catch (...) { return false; }
}
}

int main(int argc, char** argv) {
    uint32_t intervalMs = 250;
    uint32_t samples = 0; // 0 = until Ctrl+C
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--interval-ms" && i+1 < argc) {
            if (!parsePositive(argv[++i], intervalMs)) return 2;
        } else if (arg == "--samples" && i+1 < argc) {
            if (!parsePositive(argv[++i], samples)) return 2;
        } else {
            std::cerr << "Usage: aoe2hd_monitor [--interval-ms 250] [--samples 30]\n";
            return 2;
        }
    }
    std::signal(SIGINT, onSignal);
    std::cout << "Monitoring AoK HD.exe (read-only), interval=" << intervalMs
              << "ms. Press Ctrl+C to stop.\n";
    bool seen = false;
    aoe2hd::MonitorState previous;
    const auto start = std::chrono::steady_clock::now();
    for (uint32_t count = 0; !stopped && (!samples || count < samples); ++count) {
        // Re-resolve the process and module each time. Works across game restarts.
        aoe2hd::ProcessMemory memory;
        const bool found = memory.open();
        const auto result = found ? aoe2hd::ReadWorld(memory, memory.moduleBase())
                                  : aoe2hd::WorldResult{};
        const auto current = aoe2hd::MakeMonitorState(found, result);
        if (!seen || aoe2hd::HasStateChanged(previous, current)) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            std::cout << "[" << elapsed << " ms] " << aoe2hd::DescribeState(current) << std::endl;
            previous = current;
            seen = true;
        }
        if (!stopped && (!samples || count+1 < samples))
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
    return 0;
}
