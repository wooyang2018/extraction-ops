// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionOpsTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionOpsTypes)

bool FExtractionStateRules::CanTransitionMatch(EExtractionMatchState From, EExtractionMatchState To)
{
	return (From == EExtractionMatchState::Loading && To == EExtractionMatchState::InRaid)
		|| (From == EExtractionMatchState::InRaid && To == EExtractionMatchState::Completed);
}

bool FExtractionStateRules::CanTransitionRun(EExtractionRunState From, EExtractionRunState To)
{
	switch (From)
	{
	case EExtractionRunState::InRaid:
		return To == EExtractionRunState::Extracting
			|| To == EExtractionRunState::Dead
			|| To == EExtractionRunState::Abandoned;
	case EExtractionRunState::Extracting:
		return To == EExtractionRunState::InRaid
			|| To == EExtractionRunState::Extracted
			|| To == EExtractionRunState::Dead
			|| To == EExtractionRunState::Abandoned;
	default:
		return false;
	}
}

bool FExtractionStateRules::CanTransitionTerminal(EExtractionTerminalState From, EExtractionTerminalState To)
{
	return (From == EExtractionTerminalState::Idle && To == EExtractionTerminalState::Activating)
		|| (From == EExtractionTerminalState::Activating
			&& (To == EExtractionTerminalState::Idle || To == EExtractionTerminalState::Activated));
}

bool FExtractionStateRules::CanTransitionZone(EExtractionZoneState From, EExtractionZoneState To)
{
	switch (From)
	{
	case EExtractionZoneState::Locked:
		return To == EExtractionZoneState::Available;
	case EExtractionZoneState::Available:
		return To == EExtractionZoneState::Locked || To == EExtractionZoneState::Countdown;
	case EExtractionZoneState::Countdown:
		return To == EExtractionZoneState::Locked
			|| To == EExtractionZoneState::Extracted
			|| To == EExtractionZoneState::Cancelled;
	case EExtractionZoneState::Extracted:
	case EExtractionZoneState::Cancelled:
		return To == EExtractionZoneState::Locked || To == EExtractionZoneState::Available;
	default:
		return false;
	}
}

EExtractionThreatLevel FExtractionStateRules::GetThreatLevelForTerminalCount(int32 ActivatedTerminalCount)
{
	switch (FMath::Clamp(ActivatedTerminalCount, 0, 3))
	{
	case 1:
		return EExtractionThreatLevel::Low;
	case 2:
		return EExtractionThreatLevel::High;
	case 3:
		return EExtractionThreatLevel::Critical;
	default:
		return EExtractionThreatLevel::None;
	}
}

float FExtractionStateRules::GetRewardMultiplier(EExtractionThreatLevel ThreatLevel)
{
	switch (ThreatLevel)
	{
	case EExtractionThreatLevel::Low:
		return 1.25f;
	case EExtractionThreatLevel::High:
		return 1.6f;
	case EExtractionThreatLevel::Critical:
		return 2.0f;
	default:
		return 1.0f;
	}
}
