#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace aoe2hd {
struct SelectionPointerEvent {
 uint32_t offset=0,pointer=0,hpAddress=0,rawHp=0;
 float hp=0.0f;
 bool pointerReadable=false,hpReadable=false,hpPlausible=false;
 std::string kind;
};
class SelectionPointerMonitor {
public:
 std::vector<SelectionPointerEvent> observe(const MemoryReader& memory,
   uint32_t playerBase,bool phaseChanged=false);
 void reset(){previous_.clear();}
private:
 std::map<uint32_t,SelectionPointerEvent> previous_;
};
}
