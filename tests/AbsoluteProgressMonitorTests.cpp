#include "AbsoluteProgressMonitor.h"
#include <cassert>
#include <cstring>
#include <map>
#include <iostream>
using namespace aoe2hd;
struct Fake:MemoryReader {
 std::map<uint32_t,unsigned char> data;
 bool read(uint32_t a,void* out,size_t n)const override {
  for(size_t i=0;i<n;++i)if(!data.count(a+static_cast<uint32_t>(i)))return false;
  auto* p=static_cast<unsigned char*>(out);
  for(size_t i=0;i<n;++i)p[i]=data.at(a+static_cast<uint32_t>(i));
  return true;
 }
};
int main(){
 Fake m;AbsoluteProgressMonitor monitor;
 const uint32_t a=0x10000;
 m.data[a]=66;m.data[a+1]=0;m.data[a+2]=0;m.data[a+3]=0;
 auto s=monitor.observe(m,{a});
 assert(s.size()==1 && s[0].kind=="initial" && s[0].value16==66 && s[0].value32==66);
 assert(monitor.observe(m,{a}).empty());
 m.data[a]=67;
 s=monitor.observe(m,{a});
 assert(s.size()==1 && s[0].kind=="change" && s[0].value16==67);
 s=monitor.observe(m,{a},true);
 assert(s.size()==1 && s[0].kind=="phase");
 m.data.erase(a+2);m.data.erase(a+3);
 s=monitor.observe(m,{a});
 assert(s.size()==1 && s[0].kind=="status" && s[0].readable16 && !s[0].readable32);
 m.data.erase(a);
 s=monitor.observe(m,{a});
 assert(s.size()==1 && s[0].kind=="status" && !s[0].readable16);
 monitor.reset();
 assert(monitor.observe(m,{a})[0].kind=="initial");
 std::cout<<"Absolute progress monitor tests passed\n";
}
