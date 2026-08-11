// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExtractionOpsTypes.h"

#include "ExtractionZone.generated.h"

class AGameStateBase;
class UBoxComponent;
class UExtractionMatchStateComponent;
class UExtractionRunStateComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FExtractionZoneStateChanged,
	EExtractionZoneState, OldState,
	EExtractionZoneState, NewState);

/** Server-authoritative extraction volume. Countdown progress lives on each player's RunState. */
UCLASS(BlueprintType, Blueprintable)
class EXTRACTIONOPSRUNTIME_API AExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	AExtractionZone(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Extraction")
	EExtractionZoneState GetZoneState() const { return ZoneState; }

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Extraction")
	FName GetZoneId() const { return ZoneId; }

	UPROPERTY(BlueprintAssignable, Category="Extraction Ops|Extraction")
	FExtractionZoneStateChanged OnZoneStateChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Extraction Ops|Extraction")
	TObjectPtr<UBoxComponent> ExtractionVolume;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Extraction Ops|Extraction")
	FName ZoneId = TEXT("Extraction_A");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Extraction", meta=(ClampMin="1.0"))
	float ExtractionDurationSeconds = 10.0f;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UFUNCTION()
	void HandleMatchSnapshotChanged(const FExtractionMatchSnapshot& OldSnapshot,
		const FExtractionMatchSnapshot& NewSnapshot);

	UFUNCTION()
	void OnRep_ZoneState(EExtractionZoneState OldState);

	void CompleteExtraction(UExtractionRunStateComponent* RunState);
	void CancelExtraction(UExtractionRunStateComponent* RunState);
	void CancelAllExtractions();
	void RefreshZoneState();
	void SetZoneState(EExtractionZoneState NewState);
	UExtractionMatchStateComponent* FindMatchState() const;
	static UExtractionRunStateComponent* FindRunState(const AActor* Actor);

	UPROPERTY(ReplicatedUsing=OnRep_ZoneState)
	EExtractionZoneState ZoneState = EExtractionZoneState::Locked;

	UPROPERTY()
	TObjectPtr<UExtractionMatchStateComponent> MatchState;

	TMap<TWeakObjectPtr<UExtractionRunStateComponent>, FTimerHandle> ActiveCountdowns;
};
