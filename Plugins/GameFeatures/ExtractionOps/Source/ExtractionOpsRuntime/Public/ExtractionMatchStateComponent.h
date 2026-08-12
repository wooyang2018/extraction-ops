// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "ExtractionOpsTypes.h"

#include "ExtractionMatchStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FExtractionMatchSnapshotChanged,
	const FExtractionMatchSnapshot&, OldSnapshot,
	const FExtractionMatchSnapshot&, NewSnapshot);

/** Replicated, server-authoritative match state shared by all extraction gameplay systems. */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class EXTRACTIONOPSRUNTIME_API UExtractionMatchStateComponent final : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UExtractionMatchStateComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Match")
	const FExtractionMatchSnapshot& GetSnapshot() const { return Snapshot; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Match")
	bool StartRaid(FName SelectedExtractionZoneId, double RaidEndServerTime);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Match")
	bool ActivateTerminal(FName TerminalId);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Match")
	bool CompleteMatch();

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Match")
	bool IsExtractionAvailable(FName ZoneId) const;

	UPROPERTY(BlueprintAssignable, Category="Extraction Ops|Match")
	FExtractionMatchSnapshotChanged OnSnapshotChanged;

private:
	void CommitSnapshot(const FExtractionMatchSnapshot& NewSnapshot);

	UFUNCTION()
	void OnRep_Snapshot(FExtractionMatchSnapshot OldSnapshot);

	UPROPERTY(ReplicatedUsing=OnRep_Snapshot)
	FExtractionMatchSnapshot Snapshot;

	TSet<FName> ActivatedTerminalIds;
};
