// Copyright Extraction Ops. All Rights Reserved.

#include "Inventory/ExtractionWorldPickup.h"

#include "Abilities/ExtractionGameplayAbility_InteractTarget.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ExtractionRunStateComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Physics/LyraCollisionChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionWorldPickup)

AExtractionWorldPickup::AExtractionWorldPickup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = false;
	SetNetCullDistanceSquared(FMath::Square(5000.0f));
	PrimaryActorTick.bCanEverTick = false;

	InteractionVolume = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionVolume"));
	SetRootComponent(InteractionVolume);
	InteractionVolume->SetSphereRadius(70.0f);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCollisionResponseToChannel(Lyra_TraceChannel_Interaction, ECR_Block);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(InteractionVolume);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AExtractionWorldPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Item);
	DOREPLIFETIME(ThisClass, bAvailable);
}

void AExtractionWorldPickup::GatherInteractionOptions(const FInteractionQuery&,
	FInteractionOptionBuilder& InteractionBuilder)
{
	if (!bAvailable)
	{
		return;
	}
	FInteractionOption Option;
	Option.Text = NSLOCTEXT("ExtractionOps", "Pickup", "拾取");
	Option.InteractionAbilityToGrant = UExtractionGameplayAbility_InteractTarget::StaticClass();
	InteractionBuilder.AddInteractionOption(Option);
}

bool AExtractionWorldPickup::InitializePickup(const FExtractionRaidItem& InItem)
{
	if (!HasAuthority() || bAvailable || !InItem.IsValid())
	{
		return false;
	}
	Item = InItem;
	bAvailable = true;
	ForceNetUpdate();
	return true;
}

EExtractionInventoryResult AExtractionWorldPickup::TryClaim(AController* RequestingController, FGuid RequestId)
{
	EExtractionInventoryResult Failure = EExtractionInventoryResult::InvalidRequest;
	if (!HasAuthority() || !ValidateClaim(RequestingController, Failure))
	{
		return HasAuthority() ? Failure : EExtractionInventoryResult::InvalidRequest;
	}

	APlayerState* PlayerState = RequestingController->PlayerState;
	UExtractionInventoryComponent* Inventory = PlayerState
		? PlayerState->FindComponentByClass<UExtractionInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		return EExtractionInventoryResult::InvalidState;
	}

	const EExtractionInventoryResult Result = Inventory->TryAddItem(Item, RequestId);
	if (Result == EExtractionInventoryResult::Success)
	{
		bAvailable = false;
		SetActorEnableCollision(false);
		ForceNetUpdate();
		Destroy();
	}
	return Result;
}

bool AExtractionWorldPickup::ValidateClaim(const AController* RequestingController,
	EExtractionInventoryResult& OutFailure) const
{
	if (!bAvailable)
	{
		OutFailure = EExtractionInventoryResult::AlreadyConsumed;
		return false;
	}
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
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ExtractionPickupLOS), false, Pawn);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Pawn->GetPawnViewLocation(), GetActorLocation(),
		ECC_Visibility, Query) && Hit.GetActor() != this)
	{
		OutFailure = EExtractionInventoryResult::LineOfSightBlocked;
		return false;
	}
	OutFailure = EExtractionInventoryResult::Success;
	return true;
}
