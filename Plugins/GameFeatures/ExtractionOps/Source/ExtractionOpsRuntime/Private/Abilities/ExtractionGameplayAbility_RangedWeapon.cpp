// Copyright Epic Games, Inc. All Rights Reserved.

#include "Abilities/ExtractionGameplayAbility_RangedWeapon.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/LyraGameplayAbilityTargetData_SingleTargetHit.h"
#include "LyraGameplayTags.h"
#include "GameFramework/Pawn.h"
#include "ExtractionGameplayTags.h"
#include "Network/ExtractionNetworkValidation.h"
#include "Weapons/LyraRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionGameplayAbility_RangedWeapon)

namespace ExtractionRangedWeaponValidation
{
	static constexpr float MaxTraceStartDistanceFromAvatar = 750.0f;
	static constexpr float MaxRangeTolerance = 150.0f;
	static constexpr float MinForwardDot = -0.25f;
}

UExtractionGameplayAbility_RangedWeapon::UExtractionGameplayAbility_RangedWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ActivationBlockedTags.AddTag(ExtractionGameplayTags::Inventory_Open);
}

void UExtractionGameplayAbility_RangedWeapon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ProcessedCartridgeIds.Reset();
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

bool UExtractionGameplayAbility_RangedWeapon::ValidateTargetDataOnServer(
	const FGameplayAbilityTargetDataHandle& TargetData,
	FGameplayTag ApplicationTag,
	FString& OutFailureReason)
{
	(void)ApplicationTag;

	if (!CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
	{
		OutFailureReason = TEXT("not_authority");
		return false;
	}

	const APawn* SourcePawn = Cast<APawn>(CurrentActorInfo->AvatarActor.Get());
	const ULyraRangedWeaponInstance* Weapon = GetWeaponInstance();
	if (!SourcePawn || !Weapon || !CurrentActorInfo->AbilitySystemComponent.IsValid())
	{
		OutFailureReason = TEXT("invalid_source_or_weapon");
		return false;
	}

	if (TargetData.Num() == 0 || TargetData.Num() > Weapon->GetBulletsPerCartridge())
	{
		OutFailureReason = TEXT("invalid_pellet_count");
		return false;
	}

	const FVector AvatarLocation = SourcePawn->GetActorLocation();
	const FVector ViewDirection = SourcePawn->GetBaseAimRotation().Vector();
	const float MaxTraceDistance = Weapon->GetMaxDamageRange() + ExtractionRangedWeaponValidation::MaxRangeTolerance;
	const bool bBlockedByState = CurrentActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(LyraGameplayTags::Status_Death)
		|| CurrentActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(ExtractionGameplayTags::State_Reloading);
	int32 CartridgeId = INDEX_NONE;

	for (int32 Index = 0; Index < TargetData.Num(); ++Index)
	{
		const FGameplayAbilityTargetData* RawData = TargetData.Get(Index);
		const FLyraGameplayAbilityTargetData_SingleTargetHit* HitData =
			RawData && RawData->GetScriptStruct()->IsChildOf(FLyraGameplayAbilityTargetData_SingleTargetHit::StaticStruct())
				? static_cast<const FLyraGameplayAbilityTargetData_SingleTargetHit*>(RawData)
				: nullptr;
		if (!HitData)
		{
			OutFailureReason = TEXT("invalid_target_data_type");
			return false;
		}

		if (CartridgeId == INDEX_NONE)
		{
			CartridgeId = HitData->CartridgeID;
		}
		else if (CartridgeId != HitData->CartridgeID)
		{
			OutFailureReason = TEXT("mixed_cartridge_ids");
			return false;
		}

		const FHitResult& Hit = HitData->HitResult;
		const FVector TraceStart = Hit.TraceStart;
		const FVector TraceEnd = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.TraceEnd;
		const FVector TraceDelta = TraceEnd - TraceStart;
		FExtractionShotValidationInput ValidationInput;
		ValidationInput.bOwnedByRequestingConnection = CurrentActorInfo->PlayerController.IsValid();
		ValidationInput.bWeaponEquipped = true;
		ValidationInput.bBlockedByState = bBlockedByState;
		ValidationInput.AuthoritativeAmmo = 1; // CommitAbility remains the authoritative ammo/cost gate.
		ValidationInput.OriginErrorCentimeters = FVector::Distance(TraceStart, AvatarLocation);
		ValidationInput.MaximumOriginErrorCentimeters = ExtractionRangedWeaponValidation::MaxTraceStartDistanceFromAvatar;
		ValidationInput.DirectionDot = FVector::DotProduct(TraceDelta.GetSafeNormal(), ViewDirection);
		ValidationInput.MinimumDirectionDot = ExtractionRangedWeaponValidation::MinForwardDot;
		ValidationInput.RequestedTraceDistance = TraceDelta.Size();
		ValidationInput.MaximumTraceDistance = MaxTraceDistance;
		ValidationInput.bDuplicateRequest = ProcessedCartridgeIds.Contains(CartridgeId);
		const EExtractionShotRejectionReason Rejection = TraceStart.ContainsNaN() || TraceEnd.ContainsNaN()
			? EExtractionShotRejectionReason::InvalidOrigin
			: FExtractionNetworkValidation::ValidateShot(ValidationInput);
		if (Rejection != EExtractionShotRejectionReason::None)
		{
			OutFailureReason = FExtractionNetworkValidation::ToStableReason(Rejection);
			return false;
		}

		const AActor* HitActor = Hit.GetActor();
		if ((HitActor && (!IsValid(HitActor) || HitActor == SourcePawn)))
		{
			OutFailureReason = TEXT("invalid_hit_actor");
			return false;
		}
	}

	if (CartridgeId == INDEX_NONE || ProcessedCartridgeIds.Contains(CartridgeId))
	{
		OutFailureReason = TEXT("duplicate_cartridge");
		return false;
	}

	ProcessedCartridgeIds.Add(CartridgeId);
	return true;
}
