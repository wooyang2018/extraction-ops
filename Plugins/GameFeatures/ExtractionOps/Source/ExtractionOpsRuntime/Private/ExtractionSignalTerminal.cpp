// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionSignalTerminal.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "ExtractionMatchStateComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionSignalTerminal)

AExtractionSignalTerminal::AExtractionSignalTerminal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AExtractionSignalTerminal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TerminalState);
}

bool AExtractionSignalTerminal::BeginActivation(AController* InstigatingController)
{
	UExtractionMatchStateComponent* MatchStateComponent = FindMatchStateComponent();
	if (!HasAuthority() || !InstigatingController || TerminalId.IsNone()
		|| TerminalState != EExtractionTerminalState::Idle
		|| !MatchStateComponent
		|| MatchStateComponent->GetSnapshot().MatchState != EExtractionMatchState::InRaid)
	{
		return false;
	}

	ActivatingController = InstigatingController;
	SetTerminalState(EExtractionTerminalState::Activating);
	GetWorldTimerManager().SetTimer(
		ActivationTimerHandle,
		this,
		&ThisClass::FinishActivation,
		ActivationDuration,
		false);
	return true;
}

bool AExtractionSignalTerminal::CancelActivation()
{
	if (!HasAuthority() || TerminalState != EExtractionTerminalState::Activating)
	{
		return false;
	}

	GetWorldTimerManager().ClearTimer(ActivationTimerHandle);
	ActivatingController = nullptr;
	SetTerminalState(EExtractionTerminalState::Idle);
	return true;
}

UExtractionMatchStateComponent* AExtractionSignalTerminal::FindMatchStateComponent() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	return GameState ? GameState->FindComponentByClass<UExtractionMatchStateComponent>() : nullptr;
}

void AExtractionSignalTerminal::SetTerminalState(EExtractionTerminalState NewState)
{
	if (!FExtractionStateRules::CanTransitionTerminal(TerminalState, NewState))
	{
		return;
	}

	const EExtractionTerminalState OldState = TerminalState;
	TerminalState = NewState;
	OnTerminalStateChanged.Broadcast(OldState, TerminalState);
	ForceNetUpdate();
}

void AExtractionSignalTerminal::FinishActivation()
{
	if (!HasAuthority() || TerminalState != EExtractionTerminalState::Activating)
	{
		return;
	}

	UExtractionMatchStateComponent* MatchStateComponent = FindMatchStateComponent();
	if (MatchStateComponent && MatchStateComponent->ActivateTerminal(TerminalId))
	{
		SetTerminalState(EExtractionTerminalState::Activated);
	}
	else
	{
		SetTerminalState(EExtractionTerminalState::Idle);
	}

	ActivatingController = nullptr;
}

void AExtractionSignalTerminal::OnRep_TerminalState(EExtractionTerminalState OldState)
{
	OnTerminalStateChanged.Broadcast(OldState, TerminalState);
}
