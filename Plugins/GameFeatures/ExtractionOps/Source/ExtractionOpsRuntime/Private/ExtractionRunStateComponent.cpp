// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionRunStateComponent.h"

#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionRunStateComponent)

UExtractionRunStateComponent::UExtractionRunStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UExtractionRunStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitializeRun && GetOwner() && GetOwner()->HasAuthority() && Snapshot.RunId.IsEmpty())
	{
		InitializeRun(FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower));
	}
}

void UExtractionRunStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Snapshot);
}

bool UExtractionRunStateComponent::InitializeRun(const FString& RunId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || RunId.IsEmpty() || !Snapshot.RunId.IsEmpty())
	{
		return false;
	}

	FExtractionRunSnapshot NewSnapshot;
	NewSnapshot.RunId = RunId;
	CommitSnapshot(NewSnapshot);
	return true;
}

bool UExtractionRunStateComponent::StartExtraction(double ExtractionEndServerTime)
{
	return ExtractionEndServerTime > 0.0
		&& TransitionTo(EExtractionRunState::Extracting, ExtractionEndServerTime);
}

bool UExtractionRunStateComponent::CancelExtraction()
{
	return TransitionTo(EExtractionRunState::InRaid);
}

bool UExtractionRunStateComponent::MarkExtracted()
{
	return TransitionTo(EExtractionRunState::Extracted);
}

bool UExtractionRunStateComponent::MarkDead()
{
	return TransitionTo(EExtractionRunState::Dead);
}

bool UExtractionRunStateComponent::MarkAbandoned()
{
	return TransitionTo(EExtractionRunState::Abandoned);
}

bool UExtractionRunStateComponent::TransitionTo(EExtractionRunState NewState, double ExtractionEndServerTime)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Snapshot.RunId.IsEmpty()
		|| !FExtractionStateRules::CanTransitionRun(Snapshot.RunState, NewState))
	{
		return false;
	}

	FExtractionRunSnapshot NewSnapshot = Snapshot;
	NewSnapshot.RunState = NewState;
	NewSnapshot.ExtractionEndServerTime = ExtractionEndServerTime;
	CommitSnapshot(NewSnapshot);
	return true;
}

void UExtractionRunStateComponent::CommitSnapshot(const FExtractionRunSnapshot& NewSnapshot)
{
	const FExtractionRunSnapshot OldSnapshot = Snapshot;
	Snapshot = NewSnapshot;
	OnSnapshotChanged.Broadcast(OldSnapshot, Snapshot);

	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

void UExtractionRunStateComponent::OnRep_Snapshot(FExtractionRunSnapshot OldSnapshot)
{
	OnSnapshotChanged.Broadcast(OldSnapshot, Snapshot);
}
