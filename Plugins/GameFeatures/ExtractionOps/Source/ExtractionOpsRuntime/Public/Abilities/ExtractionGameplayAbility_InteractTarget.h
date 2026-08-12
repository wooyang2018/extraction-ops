// Copyright Extraction Ops. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/LyraGameplayAbility.h"

#include "ExtractionGameplayAbility_InteractTarget.generated.h"

/** Ability granted by Lyra interaction options; the server revalidates and commits the target action. */
UCLASS()
class EXTRACTIONOPSRUNTIME_API UExtractionGameplayAbility_InteractTarget final : public ULyraGameplayAbility
{
	GENERATED_BODY()

public:
	UExtractionGameplayAbility_InteractTarget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
