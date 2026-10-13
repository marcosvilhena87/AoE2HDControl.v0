#include "AbsoluteProgressMonitor.h"
namespace aoe2hd {
std::vector<AbsoluteProgressSample> AbsoluteProgressMonitor::observe(
 const MemoryReader& memory,const std::vector<uint32_t>& addresses,bool phaseChanged) {
 std::vector<AbsoluteProgressSample> result;
 for(uint32_t address:addresses) {
  AbsoluteProgressSample s;s.address=address;
  // Separate reads: 16-bit may be valid even if 32-bit spans an unreadable page.
  s.readable16=memory.read(address,&s.value16,2);
  s.readable32=memory.read(address,&s.value32,4);
  if(s.readable16)s.value16 &= 0xffffu;
  auto it=previous_.find(address);
  if(it==previous_.end())s.kind="initial";
  else if(s.readable16!=it->second.readable16 ||
          s.readable32!=it->second.readable32)s.kind="status";
  else if((s.readable16 && s.value16!=it->second.value16) ||
          (s.readable32 && s.value32!=it->second.value32))s.kind="change";
  else if(phaseChanged)s.kind="phase";
  if(!s.kind.empty())result.push_back(s);
  previous_[address]=s;
 }
 return result;
}
}
