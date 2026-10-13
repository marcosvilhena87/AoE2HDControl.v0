#pragma once
#include "EntityIdentityReader.h"
#include "EntityTypeDiscovery.h"
namespace aoe2hd {
struct TypeReferenceSample {
 EntityIdentity identity;
 uint32_t before=0,after=0;
 bool stable=false;
 EntityTypeSample definition;
};
TypeReferenceSample SampleTypeReference(const MemoryReader& memory,uint32_t entity,
 const std::string& label,uint32_t bytes=0x100);
}
