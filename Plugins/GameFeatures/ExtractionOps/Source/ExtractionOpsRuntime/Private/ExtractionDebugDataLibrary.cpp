// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionDebugDataLibrary.h"

#include "ExtractionMatchStateComponent.h"
#include "ExtractionRunStateComponent.h"
#include "GameModes/LyraExperienceDefinition.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "AbilitySystem/Attributes/ExtractionArmorSet.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionDebugDataLibrary)

namespace ExtractionDebugData
{
	FString GetNetModeName(ENetMode NetMode)
	{
		switch (NetMode)
		{
		case NM_Standalone:
			return TEXT("Standalone");
		case NM_DedicatedServer:
			return TEXT("DedicatedServer");
		case NM_ListenServer:
			return TEXT("ListenServer");
		case NM_Client:
			return TEXT("Client");
		default:
			return TEXT("Unknown");
		}
	}

	FString GetRoleName(ENetRole Role)
	{
		switch (Role)
		{
		case ROLE_None:
			return TEXT("None");
		case ROLE_SimulatedProxy:
			return TEXT("SimulatedProxy");
		case ROLE_AutonomousProxy:
			return TEXT("AutonomousProxy");
		case ROLE_Authority:
			return TEXT("Authority");
		default:
			return TEXT("Unknown");
		}
	}
}

bool UExtractionDebugDataLibrary::GetNetworkDebugSnapshot(
	const UObject* WorldContextObject,
	const AActor* ObservedActor,
	const APlayerController* LocalPlayerController,
	FExtractionNetworkDebugSnapshot& OutSnapshot)
{
	OutSnapshot = FExtractionNetworkDebugSnapshot();

	if (GEngine == nullptr)
	{
		return false;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (World == nullptr)
	{
		return false;
	}

	OutSnapshot.NetMode = ExtractionDebugData::GetNetModeName(World->GetNetMode());
	OutSnapshot.bHasPlayerController = IsValid(LocalPlayerController);

	const AActor* RoleActor = ObservedActor;
	if (RoleActor == nullptr && LocalPlayerController != nullptr)
	{
		RoleActor = LocalPlayerController->GetPawn();
	}

	if (RoleActor != nullptr)
	{
		OutSnapshot.bHasAuthority = RoleActor->HasAuthority();
		OutSnapshot.LocalRole = ExtractionDebugData::GetRoleName(RoleActor->GetLocalRole());
		OutSnapshot.RemoteRole = ExtractionDebugData::GetRoleName(RoleActor->GetRemoteRole());
	}
	else
	{
		OutSnapshot.bHasAuthority = World->GetNetMode() != NM_Client;
		OutSnapshot.LocalRole = TEXT("NoActor");
		OutSnapshot.RemoteRole = TEXT("NoActor");
	}

	const APlayerState* PlayerState = LocalPlayerController != nullptr ? LocalPlayerController->PlayerState : nullptr;
	if (PlayerState == nullptr && RoleActor != nullptr)
	{
		if (const APawn* Pawn = Cast<APawn>(RoleActor))
		{
			PlayerState = Pawn->GetPlayerState();
		}
	}

	OutSnapshot.bHasPlayerState = IsValid(PlayerState);

	const AGameStateBase* GameState = World->GetGameState();
	if (GameState != nullptr)
	{
		if (const ULyraExperienceManagerComponent* ExperienceManager =
			GameState->FindComponentByClass<ULyraExperienceManagerComponent>();
			ExperienceManager != nullptr && ExperienceManager->IsExperienceLoaded())
		{
			const ULyraExperienceDefinition* Experience = ExperienceManager->GetCurrentExperienceChecked();
			OutSnapshot.ExperienceId = Experience != nullptr ? Experience->GetPrimaryAssetId().ToString() : FString();
		}

		if (const UExtractionMatchStateComponent* MatchState =
			GameState->FindComponentByClass<UExtractionMatchStateComponent>())
		{
			const FExtractionMatchSnapshot& MatchSnapshot = MatchState->GetSnapshot();
			OutSnapshot.bHasMatchState = true;
			OutSnapshot.MatchState = MatchSnapshot.MatchState;
			OutSnapshot.ThreatLevel = MatchSnapshot.ThreatLevel;
		}
	}

	if (PlayerState != nullptr)
	{
		if (const UExtractionRunStateComponent* RunState =
			PlayerState->FindComponentByClass<UExtractionRunStateComponent>())
		{
			OutSnapshot.bHasRunState = true;
			OutSnapshot.RunState = RunState->GetSnapshot().RunState;
		}
		if (const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			OutSnapshot.Health = ASC->GetNumericAttribute(ULyraHealthSet::GetHealthAttribute());
			OutSnapshot.MaxHealth = ASC->GetNumericAttribute(ULyraHealthSet::GetMaxHealthAttribute());
			if (ASC->HasAttributeSetForAttribute(UExtractionArmorSet::GetArmorAttribute()))
			{
				OutSnapshot.Armor = ASC->GetNumericAttribute(UExtractionArmorSet::GetArmorAttribute());
				OutSnapshot.MaxArmor = ASC->GetNumericAttribute(UExtractionArmorSet::GetMaxArmorAttribute());
			}
		}
	}

	return true;
}
