#pragma once
#include "WorldReader.h"
namespace aoe2hd {
class ProcessMemory final : public MemoryReader {
public:
    ProcessMemory() = default;
    ProcessMemory(const ProcessMemory&) = delete;
    ProcessMemory& operator=(const ProcessMemory&) = delete;
    ~ProcessMemory() override;
    bool open();
    bool isAlive() const;
    uint32_t moduleBase() const { return moduleBase_; }
    bool read(uint32_t address, void* destination, size_t bytes) const override;
private:
    void* handle_ = nullptr;
    uint32_t moduleBase_ = 0;
};
} // namespace aoe2hd
