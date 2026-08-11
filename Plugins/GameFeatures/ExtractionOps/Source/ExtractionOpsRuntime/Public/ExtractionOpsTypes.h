// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ExtractionOpsTypes.generated.h"

UENUM(BlueprintType)
enum class EExtractionMatchState : uint8
{
	Loading,
	InRaid,
	Completed
};

UENUM(BlueprintType)
enum class EExtractionRunState : uint8
{
	InRaid,
	Extracting,
	Extracted,
	Dead,
	Abandoned
};

UENUM(BlueprintType)
enum class EExtractionTerminalState : uint8
{
	Idle,
	Activating,
	Activated
};

UENUM(BlueprintType)
enum class EExtractionZoneState : uint8
{
	Locked,
	Available,
	Countdown,
	Extracted,
	Cancelled
};

UENUM(BlueprintType)
enum class EExtractionThreatLevel : uint8
{
	None,
	Low,
	High,
	Critical
};

USTRUCT(BlueprintType)
struct EXTRACTIONOPSRUNTIME_API FExtractionMatchSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EExtractionMatchState MatchState = EExtractionMatchState::Loading;

	UPROPERTY(BlueprintReadOnly)
	EExtractionThreatLevel ThreatLevel = EExtractionThreatLevel::None;

	UPROPERTY(BlueprintReadOnly)
	int32 ActivatedTerminalCount = 0;

	UPROPERTY(BlueprintReadOnly)
	FName ActiveExtractionZoneId;

	UPROPERTY(BlueprintReadOnly)
	float RewardMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct EXTRACTIONOPSRUNTIME_API FExtractionRunSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString RunId;

	UPROPERTY(BlueprintReadOnly)
	EExtractionRunState RunState = EExtractionRunState::InRaid;

	/** Uses the replicated GameState server clock; zero when no extraction is active. */
	UPROPERTY(BlueprintReadOnly)
	double ExtractionEndServerTime = 0.0;
};

struct EXTRACTIONOPSRUNTIME_API FExtractionStateRules
{
	static bool CanTransitionMatch(EExtractionMatchState From, EExtractionMatchState To);
	static bool CanTransitionRun(EExtractionRunState From, EExtractionRunState To);
	static bool CanTransitionTerminal(EExtractionTerminalState From, EExtractionTerminalState To);
	static bool CanTransitionZone(EExtractionZoneState From, EExtractionZoneState To);
	static EExtractionThreatLevel GetThreatLevelForTerminalCount(int32 ActivatedTerminalCount);
	static float GetRewardMultiplier(EExtractionThreatLevel ThreatLevel);
};
