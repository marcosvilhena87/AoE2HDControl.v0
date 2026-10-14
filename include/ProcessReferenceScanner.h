#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <vector>
namespace aoe2hd {
struct ScanAnchor {uint32_t id=0,object=0;};
struct MultiReferenceHit {uint32_t address=0,id=0,object=0;};
std::vector<MultiReferenceHit> ScanReferenceBlock(uint32_t base,const uint8_t* data,
 uint32_t bytes,const std::vector<ScanAnchor>& anchors,uint32_t maxHits=4096);
}