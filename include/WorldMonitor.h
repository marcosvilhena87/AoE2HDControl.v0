#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <string>

namespace aoe2hd {
// Plain value; never stores pointers into the target process.
struct MonitorState {
    bool processFound = false;
    WorldStatus status = WorldStatus::ReadFailed;
    uint32_t world = 0;
    uint32_t localIndex = 0;
    uint32_t players = 0;
    uint32_t localObject = 0;
};

MonitorState MakeMonitorState(bool processFound, const WorldResult& result);
bool HasStateChanged(const MonitorState& previous, const MonitorState& current);
const char* StatusName(WorldStatus status);
std::string DescribeState(const MonitorState& state);
}
