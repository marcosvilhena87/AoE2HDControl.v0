#include "EntityHealthMonitor.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake:MemoryReader {
 std::map<uint32_t,uint32_t> data;
 bool read(uint32_t a,void* dst,size_t n)const override {
  auto it=data.find(a);
  if(n!=4||it==data.end())return false;
  std::memcpy(dst,&it->second,4);return true;
 }
 void set(uint32_t a,float f){uint32_t bits;std::memcpy(&bits,&f,4);data[a]=bits;}
};
int main(){
 Fake f;EntityHealthMonitor m;
 const uint32_t a=0x141BD998,b=0x141BDB80,c=0x141BDD68;
 f.set(a+0x34,25);f.set(b+0x34,25);f.set(c+0x34,25);
 auto rows=m.observe(f,{a,b,c});
 assert(rows.size()==3&&rows[0].hp==25&&rows[2].address==c+0x34);
 assert(m.observe(f,{a,b,c}).empty());
 f.set(a+0x34,40);f.set(b+0x34,40);f.set(c+0x34,40);
 rows=m.observe(f,{a,b,c});
 assert(rows.size()==3&&rows[1].kind=="change"&&rows[1].hp==40);
 f.set(b+0x34,18);
 rows=m.observe(f,{a,b,c});
 assert(rows.size()==1&&rows[0].base==b&&rows[0].hp==18);
 rows=m.observe(f,{a,b,c},true);
 assert(rows.size()==3&&rows[0].kind=="phase");
 f.data.erase(c+0x34);
 rows=m.observe(f,{a,b,c});
 assert(rows.size()==1&&rows[0].kind=="status"&&!rows[0].readable);
 m.reset();rows=m.observe(f,{a});
 assert(rows.size()==1&&rows[0].kind=="initial");
 std::cout<<"Entity health monitor tests passed\n";
}
