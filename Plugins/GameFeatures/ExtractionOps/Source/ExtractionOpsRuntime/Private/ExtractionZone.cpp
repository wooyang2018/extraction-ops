// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionZone.h"

#include "Components/BoxComponent.h"
#include "ExtractionMatchStateComponent.h"
#include "ExtractionRunStateComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionZone)

AExtractionZone::AExtractionZone(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	ExtractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionVolume"));
	SetRootComponent(ExtractionVolume);
	ExtractionVolume->SetBoxExtent(FVector(250.0, 250.0, 150.0));
	ExtractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ExtractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	ExtractionVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AExtractionZone::BeginPlay()
{
	Super::BeginPlay();

	ExtractionVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleBeginOverlap);
	ExtractionVolume->OnComponentEndOverlap.AddDynamic(this, &ThisClass::HandleEndOverlap);

	MatchState = FindMatchState();
	if (MatchState)
	{
		MatchState->OnSnapshotChanged.AddDynamic(this, &ThisClass::HandleMatchSnapshotChanged);
	}
	RefreshZoneState();
}

void AExtractionZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MatchState)
	{
		MatchState->OnSnapshotChanged.RemoveDynamic(this, &ThisClass::HandleMatchSnapshotChanged);
	}
	CancelAllExtractions();
	Super::EndPlay(EndPlayReason);
}

void AExtractionZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ZoneId);
	DOREPLIFETIME(ThisClass, ZoneState);
}

bool AExtractionZone::InitializeZoneId(FName InZoneId)
{
	if (!HasAuthority() || InZoneId.IsNone())
	{
		return false;
	}
	ZoneId = InZoneId;
	ForceNetUpdate();
	return true;
}

void AExtractionZone::HandleBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!HasAuthority() || !MatchState || !MatchState->IsExtractionAvailable(ZoneId))
	{
		return;
	}

	UExtractionRunStateComponent* RunState = FindRunState(OtherActor);
	if (!RunState || ActiveCountdowns.Contains(RunState)
		|| RunState->GetSnapshot().RunState != EExtractionRunState::InRaid)
	{
		return;
	}

	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const double ServerNow = GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	if (!RunState->StartExtraction(ServerNow + ExtractionDurationSeconds))
	{
		return;
	}

	FTimerHandle& TimerHandle = ActiveCountdowns.Add(RunState);
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &ThisClass::CompleteExtraction, RunState);
	GetWorldTimerManager().SetTimer(TimerHandle, TimerDelegate, ExtractionDurationSeconds, false);
	SetZoneState(EExtractionZoneState::Countdown);
}

void AExtractionZone::HandleEndOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (HasAuthority())
	{
		CancelExtraction(FindRunState(OtherActor));
	}
}

void AExtractionZone::HandleMatchSnapshotChanged(const FExtractionMatchSnapshot&,
	const FExtractionMatchSnapshot&)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!MatchState || !MatchState->IsExtractionAvailable(ZoneId))
	{
		CancelAllExtractions();
	}
	RefreshZoneState();
}

void AExtractionZone::CompleteExtraction(UExtractionRunStateComponent* RunState)
{
	if (!HasAuthority() || !RunState || !ActiveCountdowns.Contains(RunState))
	{
		return;
	}

	ActiveCountdowns.Remove(RunState);
	const APlayerState* PlayerState = Cast<APlayerState>(RunState->GetOwner());
	const APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
	if (MatchState && MatchState->IsExtractionAvailable(ZoneId)
		&& Pawn && ExtractionVolume->IsOverlappingActor(Pawn))
	{
		if (RunState->MarkExtracted())
		{
			SetZoneState(EExtractionZoneState::Extracted);
		}
	}
	else
	{
		RunState->CancelExtraction();
	}
	RefreshZoneState();
}

void AExtractionZone::CancelExtraction(UExtractionRunStateComponent* RunState)
{
	if (!RunState)
	{
		return;
	}

	if (FTimerHandle* TimerHandle = ActiveCountdowns.Find(RunState))
	{
		GetWorldTimerManager().ClearTimer(*TimerHandle);
		ActiveCountdowns.Remove(RunState);
		RunState->CancelExtraction();
		SetZoneState(EExtractionZoneState::Cancelled);
		RefreshZoneState();
	}
}

void AExtractionZone::CancelAllExtractions()
{
	TArray<TWeakObjectPtr<UExtractionRunStateComponent>> RunStates;
	ActiveCountdowns.GetKeys(RunStates);
	for (const TWeakObjectPtr<UExtractionRunStateComponent>& RunState : RunStates)
	{
		CancelExtraction(RunState.Get());
	}
}

void AExtractionZone::RefreshZoneState()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!ActiveCountdowns.IsEmpty())
	{
		SetZoneState(EExtractionZoneState::Countdown);
	}
	else if (MatchState && MatchState->IsExtractionAvailable(ZoneId))
	{
		SetZoneState(EExtractionZoneState::Available);
	}
	else
	{
		SetZoneState(EExtractionZoneState::Locked);
	}
}

void AExtractionZone::SetZoneState(EExtractionZoneState NewState)
{
	if (ZoneState == NewState
		|| !FExtractionStateRules::CanTransitionZone(ZoneState, NewState))
	{
		return;
	}

	const EExtractionZoneState OldState = ZoneState;
	ZoneState = NewState;
	OnZoneStateChanged.Broadcast(OldState, ZoneState);
	ForceNetUpdate();
}

void AExtractionZone::OnRep_ZoneState(EExtractionZoneState OldState)
{
	OnZoneStateChanged.Broadcast(OldState, ZoneState);
}

UExtractionMatchStateComponent* AExtractionZone::FindMatchState() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	return GameState ? GameState->FindComponentByClass<UExtractionMatchStateComponent>() : nullptr;
}

UExtractionRunStateComponent* AExtractionZone::FindRunState(const AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : Cast<APlayerState>(Actor);
	return PlayerState ? PlayerState->FindComponentByClass<UExtractionRunStateComponent>() : nullptr;
}
