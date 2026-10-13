#pragma once
#include "WorldReader.h"
#include <cstdint>
#include <vector>
namespace aoe2hd {
struct EntityIdentity {
 uint32_t object=0, signature=0, candidateId=0, candidateOwner=0, candidateTypeRef=0;
 bool readable=false;
};
EntityIdentity ReadEntityIdentity(const MemoryReader& memory,uint32_t object);
enum class OwnerRelation { Unknown, SameCandidate, DifferentCandidate };
OwnerRelation CompareCandidateOwners(const EntityIdentity& a,const EntityIdentity& b);
}
