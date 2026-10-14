#include "VillagerObservationLabels.h"
#include <cassert>
#include <cstring>
#include <iostream>
int main(){
 const char* roles[]={"villager","gatherer","shepherd","hunter","fisher","gold_miner",
 "lumberjack","stone_miner","farmer","builder"};
 for(int i=0;i<10;++i){
  auto p=aoe2hd::VillagerHudRoleKey(static_cast<char>('0'+i));
  assert(p&&std::strcmp(p,roles[i])==0);
 }
 assert(aoe2hd::VillagerHudRoleKey('z')==nullptr);
 assert(std::strcmp(aoe2hd::VillagerActionKey('s'),"stopped")==0);
 assert(std::strcmp(aoe2hd::VillagerActionKey('u'),"attacking_unit")==0);
 assert(std::strcmp(aoe2hd::VillagerActionKey('w'),"attacking_wolf")==0);
 assert(aoe2hd::VillagerActionKey('0')==nullptr);
 std::cout<<"Villager observation labels passed\n";
}
