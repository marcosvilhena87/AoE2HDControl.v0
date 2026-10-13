#include "EntityIdentityReader.h"
namespace aoe2hd {
EntityIdentity ReadEntityIdentity(const MemoryReader& memory,uint32_t object){
 EntityIdentity s;s.object=object;
 if(object<0x10000u || object>UINT32_MAX-0x10u || (object&3u))return s;
 // Small bounded read; all-or-nothing to avoid partially valid identity.
 uint32_t values[5]={};
 if(!memory.read(object,values,sizeof(values)))return s;
 s.signature=values[0];s.candidateId=values[2];
 s.candidateTypeRef=values[3];s.candidateOwner=values[4];
 s.readable=true;return s;
}
EntityOwnership ClassifyEntityOwnership(const EntityIdentity& entity,const WorldResult& world){
 if(!entity.readable||!entity.candidateOwner||!world.ready())return EntityOwnership::Unknown;
 const auto* local=GetLocalPlayer(world);
 if(!local||!local->object)return EntityOwnership::Unknown;
 if(entity.candidateOwner==local->object)return EntityOwnership::Local;
 for(size_t i=0;i<world.snapshot.players.size();++i){
  if(i==world.snapshot.localIndex)continue;
  if(world.snapshot.players[i].object==entity.candidateOwner)
   return EntityOwnership::OtherPlayer;
 }
 return EntityOwnership::Unmatched;
}
const char* EntityOwnershipName(EntityOwnership ownership){
 switch(ownership){
 case EntityOwnership::Local:return "local";
 case EntityOwnership::OtherPlayer:return "other_player";
 case EntityOwnership::Unmatched:return "unmatched";
 case EntityOwnership::Unknown:return "unknown";
 }
 return "unknown";
}
OwnerRelation CompareCandidateOwners(const EntityIdentity& a,const EntityIdentity& b){
 if(!a.readable||!b.readable||!a.candidateOwner||!b.candidateOwner)
  return OwnerRelation::Unknown;
 return a.candidateOwner==b.candidateOwner ?
  OwnerRelation::SameCandidate:OwnerRelation::DifferentCandidate;
}
}
