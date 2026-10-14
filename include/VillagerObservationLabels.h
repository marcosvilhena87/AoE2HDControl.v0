#pragma once
namespace aoe2hd {
// User-observed HUD role, not inferred from type IDs.
const char* VillagerHudRoleKey(char key);
// User-observed action, independent of displayed HUD role.
const char* VillagerActionKey(char key);
}