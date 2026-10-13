#include "SelectionPointerMonitor.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
using namespace aoe2hd;
struct Fake:MemoryReader {
 std::map<uint32_t,uint32_t> words;
 bool read(uint32_t a,void* p,size_t n)const override{
  auto it=words.find(a);if(n!=4||it==words.end())return false;
  std::memcpy(p,&it->second,4);return true;
 }
 void setHp(uint32_t a,float hp){uint32_t bits=0;std::memcpy(&bits,&hp,4);words[a]=bits;}
};
int main(){
 Fake f;SelectionPointerMonitor mon;const uint32_t player=0x30000,a=0x50000,b=0x51000;
 for(uint32_t o=0x15C;o<=0x16C;o+=4)f.words[player+o]=0;
 f.words[player+0x15C]=a;f.words[player+0x160]=a;
 f.setHp(a+0x34,40);
 auto v=mon.observe(f,player);
 assert(v.size()==5&&v[0].hp==40&&v[0].hpPlausible);
 assert(mon.observe(f,player).empty());
 f.words[player+0x15C]=b;f.setHp(b+0x34,25);
 v=mon.observe(f,player);
 assert(v.size()==1&&v[0].kind=="pointer_change"&&v[0].hp==25);
 f.setHp(a+0x34,38);
 v=mon.observe(f,player);
 assert(v.size()==1&&v[0].offset==0x160&&v[0].kind=="hp_change");
 f.words[player+0x15C]=0;
 v=mon.observe(f,player);
 assert(v.size()==1&&v[0].kind=="pointer_clear");
 f.words.erase(player+0x164);
 v=mon.observe(f,player);
 assert(v.size()==1&&v[0].kind=="read_status");
 v=mon.observe(f,player,true);
 assert(v.size()==5);
 mon.reset();v=mon.observe(f,player);
 assert(v.size()==5&&v[0].kind=="initial");
 std::cout<<"Selection pointer monitor tests passed\n";
}
