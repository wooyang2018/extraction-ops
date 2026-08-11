// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "ExtractionOpsTypes.h"
#include "GameFramework/Actor.h"

#include "ExtractionSignalTerminal.generated.h"

class UExtractionMatchStateComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FExtractionTerminalStateChanged,
	EExtractionTerminalState, OldState,
	EExtractionTerminalState, NewState);

/** World-facing signal terminal. Interaction abilities call BeginActivation on the server. */
UCLASS(BlueprintType, Blueprintable)
class EXTRACTIONOPSRUNTIME_API AExtractionSignalTerminal : public AActor
{
	GENERATED_BODY()

public:
	AExtractionSignalTerminal(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Terminal")
	bool BeginActivation(AController* InstigatingController);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Extraction Ops|Terminal")
	bool CancelActivation();

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Terminal")
	EExtractionTerminalState GetTerminalState() const { return TerminalState; }

	UFUNCTION(BlueprintPure, Category="Extraction Ops|Terminal")
	FName GetTerminalId() const { return TerminalId; }

	UPROPERTY(BlueprintAssignable, Category="Extraction Ops|Terminal")
	FExtractionTerminalStateChanged OnTerminalStateChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Extraction Ops|Terminal")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Extraction Ops|Terminal")
	FName TerminalId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Extraction Ops|Terminal", meta=(ClampMin="0.1"))
	float ActivationDuration = 2.0f;

private:
	UExtractionMatchStateComponent* FindMatchStateComponent() const;
	void SetTerminalState(EExtractionTerminalState NewState);
	void FinishActivation();

	UFUNCTION()
	void OnRep_TerminalState(EExtractionTerminalState OldState);

	UPROPERTY(ReplicatedUsing=OnRep_TerminalState)
	EExtractionTerminalState TerminalState = EExtractionTerminalState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<AController> ActivatingController;

	FTimerHandle ActivationTimerHandle;
};
