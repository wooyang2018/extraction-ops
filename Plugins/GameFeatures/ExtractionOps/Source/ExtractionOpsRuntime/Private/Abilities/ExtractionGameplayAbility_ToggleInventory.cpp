// Copyright Extraction Ops. All Rights Reserved.

#include "Abilities/ExtractionGameplayAbility_ToggleInventory.h"

#include "AbilitySystemComponent.h"
#include "ExtractionGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionGameplayAbility_ToggleInventory)

UExtractionGameplayAbility_ToggleInventory::UExtractionGameplayAbility_ToggleInventory(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
	ActivationPolicy = ELyraAbilityActivationPolicy::OnInputTriggered;
}

void UExtractionGameplayAbility_ToggleInventory::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	if (UAbilitySystemComponent* AbilitySystem = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		AbilitySystem->SetLooseGameplayTagCount(ExtractionGameplayTags::Inventory_Open,
			AbilitySystem->HasMatchingGameplayTag(ExtractionGameplayTags::Inventory_Open) ? 0 : 1);
	}
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
