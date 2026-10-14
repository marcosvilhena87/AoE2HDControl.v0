#include "GatheringProgressDetector.h"
#include <cassert>
#include <iostream>
using namespace aoe2hd;
int main(){
 CarryReading a;a.valid=true;a.object=100;a.id=2236;a.owner=200;a.definition=300;a.load=5;
 CarryReading b=a;b.load=5.25f;
 assert(CompareGatheringProgress(a,b)==GatheringProgress::Increased);
 b.load=5;
 assert(CompareGatheringProgress(a,b)==GatheringProgress::Unchanged);
 b.load=0;
 assert(CompareGatheringProgress(a,b)==GatheringProgress::Cleared);
 b.load=4;
 assert(CompareGatheringProgress(a,b)==GatheringProgress::Decreased);
 b=a;b.definition=301;
 assert(CompareGatheringProgress(a,b)==GatheringProgress::IdentityChanged);
 b=a;b.id=2234;
 assert(CompareGatheringProgress(a,b)==GatheringProgress::IdentityChanged);
 b=a;b.valid=false;
 assert(CompareGatheringProgress(a,b)==GatheringProgress::Unreadable);
 assert(CompareGatheringProgress({},a)==GatheringProgress::Baseline);
 std::cout<<"Gathering progress tests passed\n";
}