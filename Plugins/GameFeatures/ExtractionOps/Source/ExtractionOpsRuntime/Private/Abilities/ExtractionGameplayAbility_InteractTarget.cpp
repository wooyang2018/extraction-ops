// Copyright Extraction Ops. All Rights Reserved.

#include "Abilities/ExtractionGameplayAbility_InteractTarget.h"

#include "ExtractionSignalTerminal.h"
#include "Inventory/ExtractionLootContainer.h"
#include "Inventory/ExtractionWorldPickup.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionGameplayAbility_InteractTarget)

DEFINE_LOG_CATEGORY_STATIC(LogExtractionInteraction, Log, All);

UExtractionGameplayAbility_InteractTarget::UExtractionGameplayAbility_InteractTarget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UExtractionGameplayAbility_InteractTarget::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bool bCommitted = false;
	if (ActorInfo && ActorInfo->IsNetAuthority() && TriggerEventData)
	{
		AActor* Target = const_cast<AActor*>(ToRawPtr(TriggerEventData->Target));
		AController* Controller = ActorInfo->PlayerController.Get();
		const FGuid RequestId = FGuid::NewGuid();
		if (AExtractionSignalTerminal* Terminal = Cast<AExtractionSignalTerminal>(Target))
		{
			bCommitted = Terminal->BeginActivation(Controller);
		}
		else if (AExtractionWorldPickup* Pickup = Cast<AExtractionWorldPickup>(Target))
		{
			bCommitted = Pickup->TryClaim(Controller, RequestId) == EExtractionInventoryResult::Success;
		}
		else if (AExtractionLootContainer* Container = Cast<AExtractionLootContainer>(Target))
		{
			bCommitted = Container->TryClaimFirstAvailable(Controller, RequestId)
				== EExtractionInventoryResult::Success;
		}
	}

	UE_LOG(LogExtractionInteraction, Log, TEXT("event=interaction_commit ability=%s authority=%s result=%s"),
		*GetName(), ActorInfo && ActorInfo->IsNetAuthority() ? TEXT("true") : TEXT("false"),
		bCommitted ? TEXT("accepted") : TEXT("not_committed"));
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bCommitted);
}
