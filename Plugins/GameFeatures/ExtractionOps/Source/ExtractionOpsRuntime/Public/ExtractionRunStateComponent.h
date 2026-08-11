// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Components/PlayerStateComponent.h"
#include "ExtractionOpsTypes.h"

#include "ExtractionRunStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FExtractionRunSnapshotChanged,
	const FExtractionRunSnapshot&, OldSnapshot,
	const FExtractionRunSnapshot&, NewSnapshot);

/** Per-player authoritative raid lifecycle state. Add this component to PlayerState through the Experience. */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class EXTRACTIONOPSRUNTIME_API UExtractionRunStateComponent final : public UPlayerStateComponent
{
	GENERATED_BODY()

public:
	UExtractionRunStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Run")
	const FExtractionRunSnapshot& GetSnapshot() const { return Snapshot; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Run")
	bool InitializeRun(const FString& RunId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Run")
	bool StartExtraction(double ExtractionEndServerTime);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Run")
	bool CancelExtraction();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Run")
	bool MarkExtracted();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Run")
	bool MarkDead();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Run")
	bool MarkAbandoned();

	UPROPERTY(BlueprintAssignable, Category="Extraction Ops|Run")
	FExtractionRunSnapshotChanged OnSnapshotChanged;

	/** Development fallback until the backend supplies the authoritative run_id. */
	UPROPERTY(EditDefaultsOnly, Category="Extraction Ops|Run")
	bool bAutoInitializeRun = true;

private:
	bool TransitionTo(EExtractionRunState NewState, double ExtractionEndServerTime = 0.0);
	void CommitSnapshot(const FExtractionRunSnapshot& NewSnapshot);

	UFUNCTION()
	void OnRep_Snapshot(FExtractionRunSnapshot OldSnapshot);

	UPROPERTY(ReplicatedUsing=OnRep_Snapshot)
	FExtractionRunSnapshot Snapshot;
};
