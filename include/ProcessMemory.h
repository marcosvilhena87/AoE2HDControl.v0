#pragma once
#include "WorldReader.h"
#include <vector>
namespace aoe2hd {
struct ReadableMemoryRegion { uint32_t base=0, bytes=0, protection=0, kind=0; };
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
    std::vector<ReadableMemoryRegion> readableRegions(uint64_t maxBytes=268435456ULL) const;
private:
    void* handle_ = nullptr;
    uint32_t moduleBase_ = 0;
};
} // namespace aoe2hd
