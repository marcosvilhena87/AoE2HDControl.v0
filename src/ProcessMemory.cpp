#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <tlhelp32.h>
#include "WorldReader.h"
#include "ProcessMemory.h"
#include <cwchar>

namespace aoe2hd {
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
