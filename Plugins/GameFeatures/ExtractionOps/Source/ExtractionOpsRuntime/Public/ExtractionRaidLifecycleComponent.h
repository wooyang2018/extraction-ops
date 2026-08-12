// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Components/PlayerStateComponent.h"
#include "ExtractionOpsTypes.h"
#include "Inventory/ExtractionInventoryComponent.h"

#include "ExtractionRaidLifecycleComponent.generated.h"

class AExtractionLootContainer;
class ULyraHealthComponent;
class UExtractionRunStateComponent;

USTRUCT(BlueprintType)
struct EXTRACTIONOPSRUNTIME_API FExtractionRunResultSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid ResultEventId;

	UPROPERTY(BlueprintReadOnly)
	FString MatchId;

	UPROPERTY(BlueprintReadOnly)
	FString RunId;

	UPROPERTY(BlueprintReadOnly)
	EExtractionRunState Result = EExtractionRunState::InRaid;

	UPROPERTY(BlueprintReadOnly)
	double EndServerTime = 0.0;

	UPROPERTY(BlueprintReadOnly)
	TArray<FExtractionRaidItem> Items;

	bool IsTerminal() const
	{
		return Result == EExtractionRunState::Extracted || Result == EExtractionRunState::Dead
			|| Result == EExtractionRunState::Abandoned;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FExtractionRunResultChanged,
	const FExtractionRunResultSnapshot&, Result);

/** Bridges Lyra death, raid inventory, RunState and the owner-only result snapshot. */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class EXTRACTIONOPSRUNTIME_API UExtractionRaidLifecycleComponent final : public UPlayerStateComponent
{
	GENERATED_BODY()

public:
	UExtractionRaidLifecycleComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Result")
	const FExtractionRunResultSnapshot& GetResultSnapshot() const { return ResultSnapshot; }

	UPROPERTY(BlueprintAssignable, Category="Extraction Ops|Result")
	FExtractionRunResultChanged OnResultChanged;

private:
	void RefreshBindings();
	void FinalizeResult(EExtractionRunState Result, const TArray<FExtractionRaidItem>& Items, FGuid EventId);
	TArray<FExtractionRaidItem> CopyInventoryItems() const;

	UFUNCTION()
	void HandleDeathStarted(AActor* OwningActor);

	UFUNCTION()
	void HandleRunSnapshotChanged(const FExtractionRunSnapshot& OldSnapshot,
		const FExtractionRunSnapshot& NewSnapshot);

	UFUNCTION()
	void OnRep_ResultSnapshot();

	UPROPERTY(ReplicatedUsing=OnRep_ResultSnapshot)
	FExtractionRunResultSnapshot ResultSnapshot;

	UPROPERTY()
	TObjectPtr<ULyraHealthComponent> BoundHealthComponent;

	UPROPERTY()
	TObjectPtr<UExtractionRunStateComponent> RunState;

	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|Result")
	TSubclassOf<AExtractionLootContainer> DeathContainerClass;

	FTimerHandle BindingTimer;
};
