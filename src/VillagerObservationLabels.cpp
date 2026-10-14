#include "VillagerObservationLabels.h"
namespace aoe2hd {
const char* VillagerHudRoleKey(char k){
 switch(k){
 case '0':return "villager";
 case '1':return "gatherer";
 case '2':return "shepherd";
 case '3':return "hunter";
 case '4':return "fisher";
 case '5':return "gold_miner";
 case '6':return "lumberjack";
 case '7':return "stone_miner";
 case '8':return "farmer";
 case '9':return "builder";
 default:return nullptr;
 }
}
const char* VillagerActionKey(char k){
 switch(k){
 case 's':return "stopped";
 case 'm':return "moving";
 case 'g':return "gathering";
 case 'b':return "building";
 case 'u':return "attacking_unit";
 case 'e':return "attacking_building";
 case 'w':return "attacking_wolf";
 case 'x':return "other";
 default:return nullptr;
 }
}
}