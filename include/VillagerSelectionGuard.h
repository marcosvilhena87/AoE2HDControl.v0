#pragma once
#include "SelectedTelemetryReader.h"
namespace aoe2hd {
enum class VillagerSelectionStatus { AcceptedCandidate, NoSelection, NotLocal, MissingType, NotKnownVillagerType };
VillagerSelectionStatus CheckVillagerCandidate(const SelectedTelemetry& sample);
const char* VillagerSelectionStatusName(VillagerSelectionStatus status);
}