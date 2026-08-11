// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionMatchStateComponent.h"

#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionMatchStateComponent)

UExtractionMatchStateComponent::UExtractionMatchStateComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UExtractionMatchStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, Snapshot);
}

bool UExtractionMatchStateComponent::StartRaid(FName SelectedExtractionZoneId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || SelectedExtractionZoneId.IsNone()
		|| !FExtractionStateRules::CanTransitionMatch(Snapshot.MatchState, EExtractionMatchState::InRaid))
	{
		return false;
	}

	FExtractionMatchSnapshot NewSnapshot = Snapshot;
	NewSnapshot.MatchState = EExtractionMatchState::InRaid;
	NewSnapshot.ActiveExtractionZoneId = SelectedExtractionZoneId;
	CommitSnapshot(NewSnapshot);
	return true;
}

bool UExtractionMatchStateComponent::ActivateTerminal(FName TerminalId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || TerminalId.IsNone()
		|| Snapshot.MatchState != EExtractionMatchState::InRaid
		|| Snapshot.ActivatedTerminalCount >= 3
		|| ActivatedTerminalIds.Contains(TerminalId))
	{
		return false;
	}

	ActivatedTerminalIds.Add(TerminalId);

	FExtractionMatchSnapshot NewSnapshot = Snapshot;
	NewSnapshot.ActivatedTerminalCount++;
	NewSnapshot.ThreatLevel = FExtractionStateRules::GetThreatLevelForTerminalCount(NewSnapshot.ActivatedTerminalCount);
	NewSnapshot.RewardMultiplier = FExtractionStateRules::GetRewardMultiplier(NewSnapshot.ThreatLevel);
	CommitSnapshot(NewSnapshot);
	return true;
}

bool UExtractionMatchStateComponent::CompleteMatch()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()
		|| !FExtractionStateRules::CanTransitionMatch(Snapshot.MatchState, EExtractionMatchState::Completed))
	{
		return false;
	}

	FExtractionMatchSnapshot NewSnapshot = Snapshot;
	NewSnapshot.MatchState = EExtractionMatchState::Completed;
	CommitSnapshot(NewSnapshot);
	return true;
}

bool UExtractionMatchStateComponent::IsExtractionAvailable(FName ZoneId) const
{
	return Snapshot.MatchState == EExtractionMatchState::InRaid
		&& Snapshot.ActivatedTerminalCount > 0
		&& !ZoneId.IsNone()
		&& Snapshot.ActiveExtractionZoneId == ZoneId;
}

void UExtractionMatchStateComponent::CommitSnapshot(const FExtractionMatchSnapshot& NewSnapshot)
{
	const FExtractionMatchSnapshot OldSnapshot = Snapshot;
	Snapshot = NewSnapshot;
	OnSnapshotChanged.Broadcast(OldSnapshot, Snapshot);

	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}

void UExtractionMatchStateComponent::OnRep_Snapshot(FExtractionMatchSnapshot OldSnapshot)
{
	OnSnapshotChanged.Broadcast(OldSnapshot, Snapshot);
}
