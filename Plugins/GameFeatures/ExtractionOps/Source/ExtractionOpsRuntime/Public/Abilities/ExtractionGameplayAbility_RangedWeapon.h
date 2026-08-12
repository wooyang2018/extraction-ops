// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Weapons/LyraGameplayAbility_RangedWeapon.h"

#include "ExtractionGameplayAbility_RangedWeapon.generated.h"

/**
 * Extraction-specific server validation layered on Lyra's predicted ranged
 * weapon pipeline. Clients still predict presentation; only the server accepts
 * target data, commits ammo, and permits the damage effect graph to run.
 */
UCLASS(Abstract)
class EXTRACTIONOPSRUNTIME_API UExtractionGameplayAbility_RangedWeapon : public ULyraGameplayAbility_RangedWeapon
{
	GENERATED_BODY()

public:
	UExtractionGameplayAbility_RangedWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	virtual bool ValidateTargetDataOnServer(
		const FGameplayAbilityTargetDataHandle& TargetData,
		FGameplayTag ApplicationTag,
		FString& OutFailureReason) override;

private:
	TSet<int32> ProcessedCartridgeIds;
};
