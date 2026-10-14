#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include "WorldReader.h"
#include "ProcessMemory.h"
#include <cwchar>
#include <algorithm>

namespace aoe2hd {
std::vector<ReadableMemoryRegion> ProcessMemory::readableRegions(uint64_t maxBytes) const {
 std::vector<ReadableMemoryRegion> regions;
 if(!handle_||!maxBytes)return regions;
 uint64_t address=0,remaining=maxBytes;
 const uint64_t limit=0x100000000ULL;
 while(address<limit&&remaining>=4){
  MEMORY_BASIC_INFORMATION info{};
  SIZE_T got=VirtualQueryEx(static_cast<HANDLE>(handle_),
   reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(address)),&info,sizeof(info));
  if(!got)break;
  uint64_t base=reinterpret_cast<uintptr_t>(info.BaseAddress);
  uint64_t end=base+static_cast<uint64_t>(info.RegionSize);
  if(end<=address)break;
  const DWORD protect=info.Protect;
  const bool accessible=info.State==MEM_COMMIT&&!(protect&PAGE_GUARD)&&
   !(protect&PAGE_NOACCESS)&&
   ((protect&0xffu)==PAGE_READONLY||(protect&0xffu)==PAGE_READWRITE||
    (protect&0xffu)==PAGE_WRITECOPY||(protect&0xffu)==PAGE_EXECUTE_READ||
    (protect&0xffu)==PAGE_EXECUTE_READWRITE||(protect&0xffu)==PAGE_EXECUTE_WRITECOPY);
  if(accessible&&base<limit){
   uint64_t size=(std::min)(end,limit)-base;
   size=(std::min)(size,remaining);
   size&=~uint64_t(3);
   if(size>=4){
    regions.push_back({static_cast<uint32_t>(base),static_cast<uint32_t>(size),
     static_cast<uint32_t>(protect),static_cast<uint32_t>(info.Type)});
    remaining-=size;
   }
  }
  address=end;
 }
 return regions;
}
ProcessMemory::~ProcessMemory() {
    if (handle_) CloseHandle(static_cast<HANDLE>(handle_));
}
bool ProcessMemory::isAlive() const {
    if (!handle_) return false;
    DWORD code = 0;
    return GetExitCodeProcess(static_cast<HANDLE>(handle_), &code) && code == STILL_ACTIVE;
}
bool ProcessMemory::read(uint32_t address, void* destination, size_t bytes) const {
    if (!handle_) return false;
    SIZE_T readBytes = 0;
    return ReadProcessMemory(static_cast<HANDLE>(handle_),
        reinterpret_cast<LPCVOID>(static_cast<uintptr_t>(address)),
        destination, bytes, &readBytes) && readBytes == bytes;
}
bool ProcessMemory::open() {
    if (handle_) return true;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W e{};
    e.dwSize = sizeof(e);
    DWORD pid = 0;
    if (Process32FirstW(snapshot, &e)) {
        do {
            if (_wcsicmp(e.szExeFile, L"AoK HD.exe") == 0) { pid = e.th32ProcessID; break; }
        } while (Process32NextW(snapshot, &e));
    }
    CloseHandle(snapshot);
    if (!pid) return false;
    HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return false;
    HANDLE modules = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (modules == INVALID_HANDLE_VALUE) { CloseHandle(h); return false; }
    MODULEENTRY32W m{};
    m.dwSize = sizeof(m);
    uintptr_t base = 0;
    if (Module32FirstW(modules, &m)) {
        do {
            if (_wcsicmp(m.szModule, L"AoK HD.exe") == 0) {
                base = reinterpret_cast<uintptr_t>(m.modBaseAddr);
                break;
            }
        } while (Module32NextW(modules, &m));
    }
    CloseHandle(modules);
    if (!base || base > UINT32_MAX) { CloseHandle(h); return false; }
    handle_ = h;
    moduleBase_ = static_cast<uint32_t>(base);
    return true;
}
} // namespace aoe2hd
#endif
