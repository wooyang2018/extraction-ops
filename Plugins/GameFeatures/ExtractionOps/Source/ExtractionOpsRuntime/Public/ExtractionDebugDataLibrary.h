// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ExtractionOpsTypes.h"

#include "ExtractionDebugDataLibrary.generated.h"

class AActor;
class APlayerController;

/** Read-only network and extraction state consumed by development HUDs. */
USTRUCT(BlueprintType)
struct EXTRACTIONOPSRUNTIME_API FExtractionNetworkDebugSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	FString NetMode;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	bool bHasAuthority = false;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	FString LocalRole;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	FString RemoteRole;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	bool bHasPlayerController = false;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	bool bHasPlayerState = false;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	FString ExperienceId;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	bool bHasMatchState = false;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	EExtractionMatchState MatchState = EExtractionMatchState::Loading;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	EExtractionThreatLevel ThreatLevel = EExtractionThreatLevel::None;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	bool bHasRunState = false;

	UPROPERTY(BlueprintReadOnly, Category="Extraction Ops|Debug")
	EExtractionRunState RunState = EExtractionRunState::InRaid;
};

/** Keeps debug UI read-only and independent from the authoritative gameplay components. */
UCLASS()
class EXTRACTIONOPSRUNTIME_API UExtractionDebugDataLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Extraction Ops|Debug", meta=(WorldContext="WorldContextObject"))
	static bool GetNetworkDebugSnapshot(
		const UObject* WorldContextObject,
		const AActor* ObservedActor,
		const APlayerController* LocalPlayerController,
		FExtractionNetworkDebugSnapshot& OutSnapshot);
};
