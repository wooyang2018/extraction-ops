// Copyright Extraction Ops. All Rights Reserved.

#include "Inventory/ExtractionLootContainer.h"

#include "Abilities/ExtractionGameplayAbility_InteractTarget.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ExtractionRunStateComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Physics/LyraCollisionChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionLootContainer)

DEFINE_LOG_CATEGORY_STATIC(LogExtractionLoot, Log, All);

AExtractionLootContainer::AExtractionLootContainer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	SetNetCullDistanceSquared(FMath::Square(8000.0f));

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	SetRootComponent(InteractionVolume);
	InteractionVolume->SetBoxExtent(FVector(80.0f, 60.0f, 50.0f));
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(Lyra_TraceChannel_Interaction, ECR_Block);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(InteractionVolume);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AExtractionLootContainer::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority() && !ContainerId.IsValid())
	{
		TArray<FExtractionRaidItem> Generated;
		for (const TSoftObjectPtr<UExtractionItemDefinition>& Definition : InitialDefinitions)
		{
			if (!Definition.IsNull())
			{
				FExtractionRaidItem& Item = Generated.AddDefaulted_GetRef();
				Item.InstanceId = FGuid::NewGuid();
				Item.Definition = Definition;
				Item.Quantity = InitialQuantityPerDefinition;
			}
		}
		InitializeItems(Generated);
	}
}

void AExtractionLootContainer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, ContainerId);
	DOREPLIFETIME(ThisClass, Items);
}

void AExtractionLootContainer::GatherInteractionOptions(const FInteractionQuery&,
	FInteractionOptionBuilder& InteractionBuilder)
{
	if (Items.IsEmpty())
	{
		return;
	}
	FInteractionOption Option;
	Option.Text = NSLOCTEXT("ExtractionOps", "LootContainer", "搜索物资");
	Option.InteractionAbilityToGrant = UExtractionGameplayAbility_InteractTarget::StaticClass();
	InteractionBuilder.AddInteractionOption(Option);
}

bool AExtractionLootContainer::InitializeItems(const TArray<FExtractionRaidItem>& InItems, FGuid InContainerId)
{
	if (!HasAuthority() || ContainerId.IsValid())
	{
		return false;
	}
	for (const FExtractionRaidItem& Item : InItems)
	{
		if (!Item.IsValid())
		{
			return false;
		}
	}
	ContainerId = InContainerId.IsValid() ? InContainerId : FGuid::NewGuid();
	Items = InItems;
	ForceNetUpdate();
	return true;
}

EExtractionInventoryResult AExtractionLootContainer::TryClaimFirstAvailable(
	AController* RequestingController, FGuid RequestId)
{
	if (Items.IsEmpty())
	{
		return EExtractionInventoryResult::ItemNotFound;
	}
	return TryClaimItem(RequestingController, Items[0].InstanceId, RequestId);
}

EExtractionInventoryResult AExtractionLootContainer::TryClaimItem(AController* RequestingController,
	FGuid ItemInstanceId, FGuid RequestId)
{
	EExtractionInventoryResult Failure = EExtractionInventoryResult::InvalidRequest;
	if (!HasAuthority() || !ValidateClaim(RequestingController, Failure))
	{
		return HasAuthority() ? Failure : EExtractionInventoryResult::InvalidRequest;
	}
	if (!RequestId.IsValid() || ProcessedRequestIds.Contains(RequestId))
	{
		return EExtractionInventoryResult::AlreadyConsumed;
	}
	const int32 ItemIndex = Items.IndexOfByPredicate([ItemInstanceId](const FExtractionRaidItem& Item)
	{
		return Item.InstanceId == ItemInstanceId;
	});
	if (!Items.IsValidIndex(ItemIndex))
	{
		return EExtractionInventoryResult::AlreadyConsumed;
	}
	APlayerState* PlayerState = RequestingController->PlayerState;
	UExtractionInventoryComponent* Inventory = PlayerState
		? PlayerState->FindComponentByClass<UExtractionInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return EExtractionInventoryResult::InvalidState;
	}

	ProcessedRequestIds.Add(RequestId);
	const FExtractionRaidItem Item = Items[ItemIndex];
	const EExtractionInventoryResult Result = Inventory->TryAddItem(Item, RequestId);
	if (Result == EExtractionInventoryResult::Success)
	{
		Items.RemoveAt(ItemIndex);
		ForceNetUpdate();
	}
	UE_LOG(LogExtractionLoot, Log,
		TEXT("event=container_claim container_id=%s item_instance_id=%s request_id=%s result=%d"),
		*ContainerId.ToString(EGuidFormats::DigitsWithHyphensLower),
		*ItemInstanceId.ToString(EGuidFormats::DigitsWithHyphensLower),
		*RequestId.ToString(EGuidFormats::DigitsWithHyphensLower), static_cast<int32>(Result));
	return Result;
}

bool AExtractionLootContainer::ValidateClaim(const AController* RequestingController,
	EExtractionInventoryResult& OutFailure) const
{
	const APawn* Pawn = RequestingController ? RequestingController->GetPawn() : nullptr;
	const APlayerState* PlayerState = RequestingController ? RequestingController->PlayerState : nullptr;
	const UExtractionRunStateComponent* RunState = PlayerState
		? PlayerState->FindComponentByClass<UExtractionRunStateComponent>() : nullptr;
	if (!Pawn || !RunState || RunState->GetSnapshot().RunState != EExtractionRunState::InRaid)
	{
		OutFailure = EExtractionInventoryResult::InvalidState;
		return false;
	}
	if (FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) > FMath::Square(MaximumClaimDistance))
	{
		OutFailure = EExtractionInventoryResult::OutOfRange;
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ExtractionContainerLOS), false, Pawn);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Pawn->GetPawnViewLocation(), GetActorLocation(),
		ECC_Visibility, Query) && Hit.GetActor() != this)
	{
		OutFailure = EExtractionInventoryResult::LineOfSightBlocked;
		return false;
	}
	OutFailure = EExtractionInventoryResult::Success;
	return true;
}
