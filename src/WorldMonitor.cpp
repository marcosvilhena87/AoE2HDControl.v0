#include "WorldMonitor.h"
#include <iomanip>
#include <sstream>

namespace aoe2hd {
MonitorState MakeMonitorState(bool processFound, const WorldResult& result) {
    MonitorState s;
    s.processFound = processFound;
    if (!processFound) return s;
    s.status = result.status;
    if (result.ready()) {
        s.world = result.snapshot.world;
        s.localIndex = result.snapshot.localIndex;
        s.players = static_cast<uint32_t>(result.snapshot.players.size());
        if (const auto* player = GetLocalPlayer(result)) s.localObject = player->object;
    }
    return s;
}

bool HasStateChanged(const MonitorState& a, const MonitorState& b) {
    return a.processFound != b.processFound || a.status != b.status ||
        a.world != b.world || a.localIndex != b.localIndex ||
        a.players != b.players || a.localObject != b.localObject;
}

const char* StatusName(WorldStatus s) {
    switch (s) {
        case WorldStatus::Ready: return "Ready";
        case WorldStatus::ReadFailed: return "ReadFailed";
        case WorldStatus::MissingOwner: return "MissingOwner";
        case WorldStatus::MissingWorld: return "MissingWorld";
        case WorldStatus::InvalidWorldVtable: return "InvalidWorldVtable";
        case WorldStatus::InvalidVector: return "InvalidVector";
        case WorldStatus::InvalidLocalIndex: return "InvalidLocalIndex";
        case WorldStatus::InvalidPlayer: return "InvalidPlayer";
    }
    return "Unknown";
}

std::string DescribeState(const MonitorState& s) {
    if (!s.processFound) return "ProcessNotFound";
    std::ostringstream out;
    out << StatusName(s.status);
    if (s.status == WorldStatus::Ready) {
        out << " | World=0x" << std::hex << std::uppercase << s.world
            << " | LocalObject=0x" << s.localObject << std::dec
            << " | localIndex=" << s.localIndex << " | players=" << s.players;
    }
    return out.str();
}
}
