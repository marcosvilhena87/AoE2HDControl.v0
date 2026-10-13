#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <string>
#include <vector>
namespace aoe2hd {
struct EntityTypeField {
 uint32_t offset=0, raw=0;
 bool readable=false;
};
struct EntityTypeSample {
 std::string label;
 uint32_t object=0;
 std::vector<EntityTypeField> fields;
};
EntityTypeSample SampleEntityType(const MemoryReader& memory, uint32_t object,
 const std::string& label, uint32_t bytes=0x100);
struct EntityTypeComparison {
 uint32_t offset=0;
 bool allReadable=false, differentiates=false;
 uint32_t distinctValues=0;
};
std::vector<EntityTypeComparison> CompareEntityTypes(const std::vector<EntityTypeSample>& samples);
}
