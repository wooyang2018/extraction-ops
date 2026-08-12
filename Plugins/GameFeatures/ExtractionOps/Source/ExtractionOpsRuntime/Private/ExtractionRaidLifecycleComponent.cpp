// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionRaidLifecycleComponent.h"

#include "Character/LyraHealthComponent.h"
#include "ExtractionMatchStateComponent.h"
#include "ExtractionRunStateComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/ExtractionLootContainer.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionRaidLifecycleComponent)

DEFINE_LOG_CATEGORY_STATIC(LogExtractionLifecycle, Log, All);

UExtractionRaidLifecycleComponent::UExtractionRaidLifecycleComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	DeathContainerClass = AExtractionLootContainer::StaticClass();
}

void UExtractionRaidLifecycleComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		UE_LOG(LogExtractionLifecycle, Log, TEXT("event=raid_lifecycle_ready owner=%s"),
			*GetNameSafe(GetOwner()));
		RefreshBindings();
		GetWorld()->GetTimerManager().SetTimer(BindingTimer, this,
			&ThisClass::RefreshBindings, 0.5f, true);
	}
}

void UExtractionRaidLifecycleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority() && RunState && !ResultSnapshot.IsTerminal())
	{
		const EExtractionRunState State = RunState->GetSnapshot().RunState;
		if (State == EExtractionRunState::InRaid || State == EExtractionRunState::Extracting)
		{
			RunState->MarkAbandoned();
		}
	}
	GetWorld()->GetTimerManager().ClearTimer(BindingTimer);
	if (BoundHealthComponent)
	{
		BoundHealthComponent->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandleDeathStarted);
	}
	if (RunState)
	{
		RunState->OnSnapshotChanged.RemoveDynamic(this, &ThisClass::HandleRunSnapshotChanged);
	}
	Super::EndPlay(EndPlayReason);
}

void UExtractionRaidLifecycleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(ThisClass, ResultSnapshot, COND_OwnerOnly);
}

void UExtractionRaidLifecycleComponent::RefreshBindings()
{
	APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	if (!PlayerState)
	{
		return;
	}
	if (!RunState)
	{
		RunState = PlayerState->FindComponentByClass<UExtractionRunStateComponent>();
		if (RunState)
		{
			RunState->OnSnapshotChanged.AddUniqueDynamic(this, &ThisClass::HandleRunSnapshotChanged);
		}
	}
	ULyraHealthComponent* CurrentHealth = ULyraHealthComponent::FindHealthComponent(PlayerState->GetPawn());
	if (CurrentHealth != BoundHealthComponent)
	{
		if (BoundHealthComponent)
		{
			BoundHealthComponent->OnDeathStarted.RemoveDynamic(this, &ThisClass::HandleDeathStarted);
		}
		BoundHealthComponent = CurrentHealth;
		if (BoundHealthComponent)
		{
			BoundHealthComponent->OnDeathStarted.AddUniqueDynamic(this, &ThisClass::HandleDeathStarted);
		}
	}
}

void UExtractionRaidLifecycleComponent::HandleDeathStarted(AActor* OwningActor)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || ResultSnapshot.IsTerminal() || !RunState)
	{
		return;
	}
	const FGuid DeathEventId = FGuid::NewGuid();
	if (!RunState->MarkDead())
	{
		return;
	}
	UExtractionInventoryComponent* Inventory = GetOwner()->FindComponentByClass<UExtractionInventoryComponent>();
	const TArray<FExtractionRaidItem> DroppedItems = Inventory
		? Inventory->RemoveAllDroppableItems(DeathEventId) : TArray<FExtractionRaidItem>();

	if (!DroppedItems.IsEmpty() && DeathContainerClass && OwningActor)
	{
		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		if (AExtractionLootContainer* Container = GetWorld()->SpawnActor<AExtractionLootContainer>(
			DeathContainerClass, OwningActor->GetActorLocation(), FRotator::ZeroRotator, Parameters))
		{
			Container->InitializeItems(DroppedItems, FGuid::NewGuid());
		}
	}
	FinalizeResult(EExtractionRunState::Dead, DroppedItems, DeathEventId);
}

void UExtractionRaidLifecycleComponent::HandleRunSnapshotChanged(const FExtractionRunSnapshot&,
	const FExtractionRunSnapshot& NewSnapshot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || ResultSnapshot.IsTerminal())
	{
		return;
	}
	if (NewSnapshot.RunState == EExtractionRunState::Extracted)
	{
		FinalizeResult(NewSnapshot.RunState, CopyInventoryItems(), FGuid::NewGuid());
	}
	else if (NewSnapshot.RunState == EExtractionRunState::Abandoned)
	{
		FinalizeResult(NewSnapshot.RunState, TArray<FExtractionRaidItem>(), FGuid::NewGuid());
	}
}

TArray<FExtractionRaidItem> UExtractionRaidLifecycleComponent::CopyInventoryItems() const
{
	TArray<FExtractionRaidItem> Items;
	if (const UExtractionInventoryComponent* Inventory =
		GetOwner()->FindComponentByClass<UExtractionInventoryComponent>())
	{
		for (const FExtractionInventorySlot& Slot : Inventory->GetSlots())
		{
			if (Slot.Item.IsValid())
			{
				Items.Add(Slot.Item);
			}
		}
	}
	return Items;
}

void UExtractionRaidLifecycleComponent::FinalizeResult(EExtractionRunState Result,
	const TArray<FExtractionRaidItem>& Items, FGuid EventId)
{
	if (ResultSnapshot.IsTerminal())
	{
		return;
	}
	const AGameStateBase* GameState = GetWorld()->GetGameState();
	const UExtractionMatchStateComponent* Match = GameState
		? GameState->FindComponentByClass<UExtractionMatchStateComponent>() : nullptr;
	ResultSnapshot.ResultEventId = EventId;
	ResultSnapshot.MatchId = Match ? Match->GetSnapshot().MatchId : FString();
	ResultSnapshot.RunId = RunState ? RunState->GetSnapshot().RunId : FString();
	ResultSnapshot.Result = Result;
	ResultSnapshot.EndServerTime = GameState
		? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
	ResultSnapshot.Items = Items;
	OnResultChanged.Broadcast(ResultSnapshot);
	GetOwner()->ForceNetUpdate();
	UE_LOG(LogExtractionLifecycle, Log,
		TEXT("event=run_result match_id=%s run_id=%s result=%d event_id=%s item_count=%d"),
		*ResultSnapshot.MatchId, *ResultSnapshot.RunId, static_cast<int32>(Result),
		*EventId.ToString(EGuidFormats::DigitsWithHyphensLower), Items.Num());
}

void UExtractionRaidLifecycleComponent::OnRep_ResultSnapshot()
{
	OnResultChanged.Broadcast(ResultSnapshot);
}
