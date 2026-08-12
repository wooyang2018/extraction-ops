// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionSignalTerminal.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Abilities/ExtractionGameplayAbility_InteractTarget.h"
#include "ExtractionMatchStateComponent.h"
#include "ExtractionRunStateComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Physics/LyraCollisionChannels.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionSignalTerminal)

AExtractionSignalTerminal::AExtractionSignalTerminal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetupAttachment(SceneRoot);
	InteractionVolume->SetSphereRadius(90.0f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(Lyra_TraceChannel_Interaction, ECR_Block);
}

void AExtractionSignalTerminal::GatherInteractionOptions(const FInteractionQuery&,
	FInteractionOptionBuilder& InteractionBuilder)
{
	if (TerminalState != EExtractionTerminalState::Idle)
	{
		return;
	}
	FInteractionOption Option;
	Option.Text = NSLOCTEXT("ExtractionOps", "ActivateTerminal", "扫描信号终端");
	Option.SubText = NSLOCTEXT("ExtractionOps", "ActivateTerminalThreat", "提高威胁并获取撤离情报");
	Option.InteractionAbilityToGrant = UExtractionGameplayAbility_InteractTarget::StaticClass();
	InteractionBuilder.AddInteractionOption(Option);
}

void AExtractionSignalTerminal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, TerminalId);
	DOREPLIFETIME(ThisClass, TerminalState);
}

bool AExtractionSignalTerminal::InitializeTerminalId(FName InTerminalId)
{
	if (!HasAuthority() || InTerminalId.IsNone() || !TerminalId.IsNone())
	{
		return false;
	}
	TerminalId = InTerminalId;
	ForceNetUpdate();
	return true;
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
	if (!IsActivationStillValid())
	{
		ActivatingController = nullptr;
		return false;
	}
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
	if (IsActivationStillValid() && MatchStateComponent && MatchStateComponent->ActivateTerminal(TerminalId))
	{
		SetTerminalState(EExtractionTerminalState::Activated);
	}
	else
	{
		SetTerminalState(EExtractionTerminalState::Idle);
	}

	ActivatingController = nullptr;
}

bool AExtractionSignalTerminal::IsActivationStillValid() const
{
	const AController* Controller = ActivatingController;
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const APlayerState* PlayerState = Controller ? Controller->PlayerState : nullptr;
	const UExtractionRunStateComponent* RunState = PlayerState
		? PlayerState->FindComponentByClass<UExtractionRunStateComponent>() : nullptr;
	if (!Pawn || !RunState || RunState->GetSnapshot().RunState != EExtractionRunState::InRaid
		|| FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation())
			> FMath::Square(MaximumActivationDistance))
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ExtractionTerminalLOS), false, Pawn);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Pawn->GetPawnViewLocation(), GetActorLocation(),
		ECC_Visibility, Query) || Hit.GetActor() == this;
}

void AExtractionSignalTerminal::OnRep_TerminalState(EExtractionTerminalState OldState)
{
	OnTerminalStateChanged.Broadcast(OldState, TerminalState);
}
