// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "AIController.h"
#include "Components/GameStateComponent.h"
#include "ExtractionOpsTypes.h"

#include "ExtractionThreatDirectorComponent.generated.h"

class UExtractionMatchStateComponent;

UENUM(BlueprintType)
enum class EExtractionEnemyRole : uint8
{
	PatrolShooter,
	Flanker,
	EliteHunter
};

USTRUCT(BlueprintType)
struct EXTRACTIONOPSRUNTIME_API FExtractionThreatWaveDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EExtractionEnemyRole Role = EExtractionEnemyRole::PatrolShooter;

	UPROPERTY(BlueprintReadOnly)
	int32 Count = 0;
};

/** Pure rules used by the runtime director and automation tests. */
struct EXTRACTIONOPSRUNTIME_API FExtractionThreatScheduleRules
{
	static FExtractionThreatWaveDecision GetWave(EExtractionThreatLevel Level);
	static uint8 GetWaveBit(EExtractionThreatLevel Level);
};

/** Starts the raid, selects the active extraction zone and schedules each Threat wave once. */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class EXTRACTIONOPSRUNTIME_API UExtractionThreatDirectorComponent final : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UExtractionThreatDirectorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category="Extraction Ops|AI")
	int32 GetScheduledWaveCount() const { return FMath::CountBits(ScheduledWaveMask); }

	UFUNCTION(BlueprintPure, Category="Extraction Ops|AI")
	int32 GetAliveEnemyCount() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|AI")
	TSoftClassPtr<AAIController> PatrolControllerClass;

	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|AI")
	TSoftClassPtr<AAIController> FlankerControllerClass;

	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|AI")
	TSoftClassPtr<AAIController> EliteControllerClass;

	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|AI", meta=(ClampMin="1", ClampMax="32"))
	int32 GlobalEnemyCap = 8;

	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|AI", meta=(ClampMin="0.0"))
	float MinimumSpawnDistanceFromPlayers = 1200.0f;

	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|Raid", meta=(ClampMin="60.0", ForceUnits="s"))
	float RaidDurationSeconds = 900.0f;

private:
	void EnsureRuntimeWorldContract();
	void InitializeRaid();
	void AuditMatchCompletion();
	void ScheduleWave(EExtractionThreatLevel ThreatLevel);
	AAIController* SpawnOne(EExtractionEnemyRole Role);
	TSubclassOf<AAIController> ResolveControllerClass(EExtractionEnemyRole Role) const;

	UFUNCTION()
	void HandleMatchSnapshotChanged(const FExtractionMatchSnapshot& OldSnapshot,
		const FExtractionMatchSnapshot& NewSnapshot);

	UPROPERTY()
	TObjectPtr<UExtractionMatchStateComponent> MatchState;

	UPROPERTY()
	TArray<TObjectPtr<AAIController>> SpawnedEnemies;

	uint8 ScheduledWaveMask = 0;
	FTimerHandle InitializationTimer;
	FTimerHandle CompletionAuditTimer;
};
