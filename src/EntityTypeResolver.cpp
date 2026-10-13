#include "EntityTypeResolver.h"
namespace aoe2hd {
TypeReferenceSample SampleTypeReference(const MemoryReader& memory,uint32_t entity,
 const std::string& label,uint32_t bytes){
 TypeReferenceSample s;
 s.identity=ReadEntityIdentity(memory,entity);
 if(!s.identity.readable)return s;
 const uint32_t ref=s.identity.candidateTypeRef;
 if(ref<0x10000u||(ref&3u)||ref>UINT32_MAX-0x400u)return s;
 s.before=ref;
 s.definition=SampleEntityType(memory,ref,label,bytes);
 uint32_t reread=0;
 if(entity<=UINT32_MAX-0x0cu&&memory.read(entity+0x0c,&reread,4)){
  s.after=reread;
  s.stable=reread==ref;
 }
 return s;
}
}
