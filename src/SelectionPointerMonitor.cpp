#include "SelectionPointerMonitor.h"
#include "EntityHealthMonitor.h"
#include <cmath>
#include <cstring>
namespace aoe2hd {
std::vector<SelectionPointerEvent> SelectionPointerMonitor::observe(
 const MemoryReader& memory,uint32_t playerBase,bool phaseChanged){
 std::vector<SelectionPointerEvent> events;
 for(uint32_t offset=0x15C;offset<=0x16C;offset+=4){
  SelectionPointerEvent cur;cur.offset=offset;
  if(playerBase>=0x10000u && playerBase<=UINT32_MAX-offset)
   cur.pointerReadable=memory.read(playerBase+offset,&cur.pointer,4);
  if(cur.pointerReadable && cur.pointer>=0x10000u &&
     (cur.pointer%4)==0 && cur.pointer<=UINT32_MAX-EntityHpFieldOffset){
   cur.hpAddress=cur.pointer+EntityHpFieldOffset;
   cur.hpReadable=memory.read(cur.hpAddress,&cur.rawHp,4);
   if(cur.hpReadable){
    std::memcpy(&cur.hp,&cur.rawHp,4);
    cur.hpPlausible=std::isfinite(cur.hp)&&cur.hp>=0.0f&&cur.hp<=100000.0f;
   }
  }
  auto it=previous_.find(offset);
  if(it==previous_.end())cur.kind="initial";
  else if(it->second.pointerReadable!=cur.pointerReadable)cur.kind="read_status";
  else if(cur.pointerReadable && it->second.pointer!=cur.pointer)
   cur.kind=cur.pointer?"pointer_change":"pointer_clear";
  else if(cur.hpReadable!=it->second.hpReadable)cur.kind="hp_status";
  else if(cur.hpReadable && cur.rawHp!=it->second.rawHp)cur.kind="hp_change";
  else if(phaseChanged)cur.kind="phase";
  if(!cur.kind.empty())events.push_back(cur);
  previous_[offset]=cur;
 }
 return events;
}
}
