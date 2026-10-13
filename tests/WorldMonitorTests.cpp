#include "WorldMonitor.h"
#include <cassert>
#include <iostream>

int main() {
    using namespace aoe2hd;
    WorldResult missing;
    const auto absent = MakeMonitorState(false, missing);
    assert(DescribeState(absent) == "ProcessNotFound");
    const auto readFailure = MakeMonitorState(true, missing);
    assert(HasStateChanged(absent, readFailure));
    assert(!HasStateChanged(readFailure, readFailure));
    WorldResult invalid;
    invalid.status = WorldStatus::InvalidVector;
    auto invalidState = MakeMonitorState(true, invalid);
    assert(DescribeState(invalidState) == "InvalidVector");
    assert(HasStateChanged(readFailure, invalidState));

    WorldResult ready;
    ready.status = WorldStatus::Ready;
    ready.snapshot.world = 0x143766B0;
    ready.snapshot.localIndex = 1;
    ready.snapshot.players.resize(3);
    ready.snapshot.players[1].object = 0x1076402C;
    auto first = MakeMonitorState(true, ready);
    assert(first.players == 3);
    assert(first.localObject == 0x1076402C);
    assert(HasStateChanged(invalidState, first));
    assert(!HasStateChanged(first, first));
    ready.snapshot.world = 0x0F5428D0;
    assert(HasStateChanged(first, MakeMonitorState(true, ready)));
    assert(HasStateChanged(first, absent));
    std::cout << "WorldMonitor tests passed\n";
}
