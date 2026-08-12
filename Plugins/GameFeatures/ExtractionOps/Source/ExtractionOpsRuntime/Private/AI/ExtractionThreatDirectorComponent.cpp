// Copyright Extraction Ops. All Rights Reserved.

#include "AI/ExtractionThreatDirectorComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "EngineUtils.h"
#include "ExtractionMatchStateComponent.h"
#include "ExtractionRunStateComponent.h"
#include "ExtractionSignalTerminal.h"
#include "ExtractionZone.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/LyraGameMode.h"
#include "Inventory/ExtractionItemDefinition.h"
#include "Inventory/ExtractionLootContainer.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionThreatDirectorComponent)

DEFINE_LOG_CATEGORY_STATIC(LogExtractionThreatDirector, Log, All);

namespace ExtractionThreatDirector
{
	bool IsFarEnoughFromHumanPlayers(const UWorld* World, const FVector& Location, float MinimumDistance)
	{
		const float MinimumDistanceSquared = FMath::Square(MinimumDistance);
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PlayerController = It->Get();
			if (PlayerController && PlayerController->PlayerState && !PlayerController->PlayerState->IsABot())
			{
				if (const APawn* HumanPawn = PlayerController->GetPawn())
				{
					if (FVector::DistSquared2D(Location, HumanPawn->GetActorLocation()) < MinimumDistanceSquared)
					{
						return false;
					}
				}
			}
		}
		return true;
	}

	bool PlaceAtSafeDistance(UWorld* World, APawn* Pawn, float MinimumDistance)
	{
		if (IsFarEnoughFromHumanPlayers(World, Pawn->GetActorLocation(), MinimumDistance))
		{
			return true;
		}

		const UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
		if (!Navigation)
		{
			return false;
		}

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PlayerController = It->Get();
			const APawn* HumanPawn = PlayerController && PlayerController->PlayerState
				&& !PlayerController->PlayerState->IsABot() ? PlayerController->GetPawn() : nullptr;
			if (!HumanPawn)
			{
				continue;
			}

			for (int32 Attempt = 0; Attempt < 12; ++Attempt)
			{
				const FVector2D Direction = FMath::RandPointInCircle(1.0f).GetSafeNormal();
				const FVector SearchOrigin = HumanPawn->GetActorLocation()
					+ FVector(Direction.X, Direction.Y, 0.0f) * FMath::FRandRange(MinimumDistance, MinimumDistance + 1600.0f);
				FNavLocation Candidate;
				if (Navigation->GetRandomReachablePointInRadius(SearchOrigin, 500.0f, Candidate)
					&& IsFarEnoughFromHumanPlayers(World, Candidate.Location, MinimumDistance))
				{
					return Pawn->SetActorLocation(Candidate.Location, false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
		}
		return false;
	}

	void ApplyRoleTuning(AAIController* Controller, EExtractionEnemyRole Role)
	{
		ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
		if (!Character)
		{
			return;
		}

		float WalkSpeed = 520.0f;
		float MaxHealth = 100.0f;
		switch (Role)
		{
		case EExtractionEnemyRole::Flanker:
			WalkSpeed = 700.0f;
			break;
		case EExtractionEnemyRole::EliteHunter:
			WalkSpeed = 480.0f;
			MaxHealth = 250.0f;
			Character->SetActorScale3D(FVector(1.08f));
			break;
		default:
			break;
		}
		Character->GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

		if (APlayerState* PlayerState = Controller->PlayerState)
		{
			if (UAbilitySystemComponent* AbilitySystem = PlayerState->FindComponentByClass<UAbilitySystemComponent>())
			{
				AbilitySystem->SetNumericAttributeBase(ULyraHealthSet::GetMaxHealthAttribute(), MaxHealth);
				AbilitySystem->SetNumericAttributeBase(ULyraHealthSet::GetHealthAttribute(), MaxHealth);
			}
		}
	}
}

FExtractionThreatWaveDecision FExtractionThreatScheduleRules::GetWave(EExtractionThreatLevel Level)
{
	switch (Level)
	{
	case EExtractionThreatLevel::Low:
		return {EExtractionEnemyRole::PatrolShooter, 2};
	case EExtractionThreatLevel::High:
		return {EExtractionEnemyRole::Flanker, 2};
	case EExtractionThreatLevel::Critical:
		return {EExtractionEnemyRole::EliteHunter, 1};
	default:
		return {};
	}
}

uint8 FExtractionThreatScheduleRules::GetWaveBit(EExtractionThreatLevel Level)
{
	return Level == EExtractionThreatLevel::None ? 0 : 1 << (static_cast<uint8>(Level) - 1);
}

UExtractionThreatDirectorComponent::UExtractionThreatDirectorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PatrolControllerClass = TSoftClassPtr<AAIController>(FSoftObjectPath(
		TEXT("/ExtractionOps/AI/B_AI_Controller_ExtractionPatrol.B_AI_Controller_ExtractionPatrol_C")));
	FlankerControllerClass = TSoftClassPtr<AAIController>(FSoftObjectPath(
		TEXT("/ExtractionOps/AI/B_AI_Controller_ExtractionFlanker.B_AI_Controller_ExtractionFlanker_C")));
	EliteControllerClass = TSoftClassPtr<AAIController>(FSoftObjectPath(
		TEXT("/ExtractionOps/AI/B_AI_Controller_ExtractionElite.B_AI_Controller_ExtractionElite_C")));
}

void UExtractionThreatDirectorComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(InitializationTimer, this,
			&ThisClass::InitializeRaid, 0.25f, false);
		GetWorld()->GetTimerManager().SetTimer(CompletionAuditTimer, this,
			&ThisClass::AuditMatchCompletion, 1.0f, true);
	}
}

void UExtractionThreatDirectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MatchState)
	{
		MatchState->OnSnapshotChanged.RemoveDynamic(this, &ThisClass::HandleMatchSnapshotChanged);
	}
	GetWorld()->GetTimerManager().ClearTimer(InitializationTimer);
	GetWorld()->GetTimerManager().ClearTimer(CompletionAuditTimer);
	Super::EndPlay(EndPlayReason);
}

void UExtractionThreatDirectorComponent::EnsureRuntimeWorldContract()
{
	int32 ExistingTerminals = 0;
	int32 ExistingZones = 0;
	int32 ExistingLootContainers = 0;
	for (TActorIterator<AExtractionSignalTerminal> It(GetWorld()); It; ++It) { ++ExistingTerminals; }
	for (TActorIterator<AExtractionZone> It(GetWorld()); It; ++It) { ++ExistingZones; }
	for (TActorIterator<AExtractionLootContainer> It(GetWorld()); It; ++It) { ++ExistingLootContainers; }

	FActorSpawnParameters Parameters;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const TSubclassOf<AExtractionSignalTerminal> TerminalClass = FSoftClassPath(
		TEXT("/ExtractionOps/World/Terminals/B_SignalTerminal.B_SignalTerminal_C")).TryLoadClass<AExtractionSignalTerminal>();
	const TSubclassOf<AExtractionZone> ZoneClass = FSoftClassPath(
		TEXT("/ExtractionOps/World/Extraction/B_ExtractionZone.B_ExtractionZone_C")).TryLoadClass<AExtractionZone>();
	const TSubclassOf<AExtractionLootContainer> LootClass = FSoftClassPath(
		TEXT("/ExtractionOps/World/Loot/B_ExtractionLootContainer.B_ExtractionLootContainer_C")).TryLoadClass<AExtractionLootContainer>();

	int32 Spawned = 0;
	if (ExistingTerminals == 0 && TerminalClass)
	{
		const struct { const TCHAR* Id; FVector Location; } Terminals[] = {
			{TEXT("Terminal_A"), {-650.0, -100.0, 160.0}},
			{TEXT("Terminal_B"), {950.0, -1750.0, 150.0}},
			{TEXT("Terminal_C"), {1150.0, 1650.0, 150.0}}};
		for (const auto& Definition : Terminals)
		{
			if (AExtractionSignalTerminal* Terminal = GetWorld()->SpawnActor<AExtractionSignalTerminal>(
				TerminalClass, Definition.Location, FRotator::ZeroRotator, Parameters))
			{
				Terminal->InitializeTerminalId(Definition.Id);
				++Spawned;
			}
		}
	}
	if (ExistingZones == 0 && ZoneClass)
	{
		const struct { const TCHAR* Id; FVector Location; } Zones[] = {
			{TEXT("Extraction_A"), {-2050.0, 150.0, 150.0}},
			{TEXT("Extraction_B"), {2000.0, 150.0, 150.0}}};
		for (const auto& Definition : Zones)
		{
			if (AExtractionZone* Zone = GetWorld()->SpawnActor<AExtractionZone>(
				ZoneClass, Definition.Location, FRotator::ZeroRotator, Parameters))
			{
				Zone->InitializeZoneId(Definition.Id);
				++Spawned;
			}
		}
	}
	if (ExistingLootContainers == 0 && LootClass)
	{
		const struct { const TCHAR* DefinitionPath; FVector Location; } Loot[] = {
			{TEXT("/ExtractionOps/Items/DA_Valuable_ScrapMetal.DA_Valuable_ScrapMetal"), {-900.0, -1700.0, 150.0}},
			{TEXT("/ExtractionOps/Items/DA_Item_Medkit.DA_Item_Medkit"), {900.0, -1700.0, 150.0}},
			{TEXT("/ExtractionOps/Items/DA_Valuable_OpticModule.DA_Valuable_OpticModule"), {-1100.0, 1600.0, 150.0}},
			{TEXT("/ExtractionOps/Items/DA_Valuable_EncryptedDrive.DA_Valuable_EncryptedDrive"), {1100.0, 1600.0, 150.0}},
			{TEXT("/ExtractionOps/Items/DA_Valuable_PrototypeChip.DA_Valuable_PrototypeChip"), {0.0, 300.0, 150.0}}};
		for (const auto& Definition : Loot)
		{
			UExtractionItemDefinition* ItemDefinition = LoadObject<UExtractionItemDefinition>(nullptr, Definition.DefinitionPath);
			if (!ItemDefinition)
			{
				continue;
			}
			if (AExtractionLootContainer* Container = GetWorld()->SpawnActor<AExtractionLootContainer>(
				LootClass, Definition.Location, FRotator::ZeroRotator, Parameters))
			{
				FExtractionRaidItem Item;
				Item.InstanceId = FGuid::NewGuid();
				Item.Definition = ItemDefinition;
				Item.Quantity = 1;
				Container->InitializeItems({Item}, FGuid::NewGuid());
				++Spawned;
			}
		}
	}

	UE_LOG(LogExtractionThreatDirector, Log,
		TEXT("event=world_contract_ready existing_terminals=%d existing_zones=%d existing_loot=%d spawned=%d"),
		ExistingTerminals, ExistingZones, ExistingLootContainers, Spawned);
}

void UExtractionThreatDirectorComponent::InitializeRaid()
{
	EnsureRuntimeWorldContract();
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	MatchState = GameState->FindComponentByClass<UExtractionMatchStateComponent>();
	if (!MatchState)
	{
		UE_LOG(LogExtractionThreatDirector, Error, TEXT("event=raid_start_failed reason=missing_match_state"));
		return;
	}
	MatchState->OnSnapshotChanged.AddUniqueDynamic(this, &ThisClass::HandleMatchSnapshotChanged);

	TArray<FName> ZoneIds;
	for (TActorIterator<AExtractionZone> It(GetWorld()); It; ++It)
	{
		if (!It->GetZoneId().IsNone())
		{
			ZoneIds.AddUnique(It->GetZoneId());
		}
	}
	ZoneIds.Sort(FNameLexicalLess());
	if (ZoneIds.IsEmpty())
	{
		UE_LOG(LogExtractionThreatDirector, Error, TEXT("event=raid_start_failed reason=no_extraction_zone"));
		return;
	}
	const int32 SelectedIndex = FMath::RandHelper(ZoneIds.Num());
	const double RaidEndServerTime = GameState->GetServerWorldTimeSeconds() + RaidDurationSeconds;
	if (MatchState->StartRaid(ZoneIds[SelectedIndex], RaidEndServerTime))
	{
		UE_LOG(LogExtractionThreatDirector, Log,
			TEXT("event=raid_started match_id=%s extraction_zone=%s raid_end_server_time=%.3f"),
			*MatchState->GetSnapshot().MatchId, *ZoneIds[SelectedIndex].ToString(), RaidEndServerTime);
	}
}

void UExtractionThreatDirectorComponent::HandleMatchSnapshotChanged(const FExtractionMatchSnapshot& OldSnapshot,
	const FExtractionMatchSnapshot& NewSnapshot)
{
	if (NewSnapshot.ThreatLevel != OldSnapshot.ThreatLevel)
	{
		ScheduleWave(NewSnapshot.ThreatLevel);
	}
}

void UExtractionThreatDirectorComponent::ScheduleWave(EExtractionThreatLevel ThreatLevel)
{
	const uint8 WaveBit = FExtractionThreatScheduleRules::GetWaveBit(ThreatLevel);
	if (WaveBit == 0 || (ScheduledWaveMask & WaveBit) != 0)
	{
		return;
	}
	ScheduledWaveMask |= WaveBit;
	const FExtractionThreatWaveDecision Decision = FExtractionThreatScheduleRules::GetWave(ThreatLevel);
	int32 Spawned = 0;
	for (int32 Index = 0; Index < Decision.Count && GetAliveEnemyCount() < GlobalEnemyCap; ++Index)
	{
		Spawned += SpawnOne(Decision.Role) ? 1 : 0;
	}
	UE_LOG(LogExtractionThreatDirector, Log,
		TEXT("event=threat_wave threat=%d role=%d requested=%d spawned=%d alive=%d"),
		static_cast<int32>(ThreatLevel), static_cast<int32>(Decision.Role), Decision.Count,
		Spawned, GetAliveEnemyCount());
}

AAIController* UExtractionThreatDirectorComponent::SpawnOne(EExtractionEnemyRole Role)
{
	TSubclassOf<AAIController> ControllerClass = ResolveControllerClass(Role);
	ALyraGameMode* GameMode = GetWorld()->GetAuthGameMode<ALyraGameMode>();
	if (!ControllerClass || !GameMode)
	{
		return nullptr;
	}
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.ObjectFlags |= RF_Transient;
	AAIController* Controller = GetWorld()->SpawnActor<AAIController>(ControllerClass,
		FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
	if (!Controller)
	{
		return nullptr;
	}
	GameMode->GenericPlayerInitialization(Controller);
	GameMode->RestartPlayer(Controller);
	if (APawn* Pawn = Controller->GetPawn())
	{
		Pawn->Tags.AddUnique(*UEnum::GetValueAsString(Role));
		if (ULyraPawnExtensionComponent* PawnExtension = Pawn->FindComponentByClass<ULyraPawnExtensionComponent>())
		{
			PawnExtension->CheckDefaultInitialization();
		}
		if (!ExtractionThreatDirector::PlaceAtSafeDistance(GetWorld(), Pawn, MinimumSpawnDistanceFromPlayers))
		{
			UE_LOG(LogExtractionThreatDirector, Warning,
				TEXT("event=enemy_spawn_rejected role=%d reason=no_safe_nav_location min_distance=%.0f"),
				static_cast<int32>(Role), MinimumSpawnDistanceFromPlayers);
			Pawn->Destroy();
			Controller->Destroy();
			return nullptr;
		}
		ExtractionThreatDirector::ApplyRoleTuning(Controller, Role);
	}
	SpawnedEnemies.Add(Controller);
	return Controller;
}

TSubclassOf<AAIController> UExtractionThreatDirectorComponent::ResolveControllerClass(EExtractionEnemyRole Role) const
{
	switch (Role)
	{
	case EExtractionEnemyRole::Flanker:
		return FlankerControllerClass.LoadSynchronous();
	case EExtractionEnemyRole::EliteHunter:
		return EliteControllerClass.LoadSynchronous();
	default:
		return PatrolControllerClass.LoadSynchronous();
	}
}

int32 UExtractionThreatDirectorComponent::GetAliveEnemyCount() const
{
	int32 AliveCount = 0;
	for (const AAIController* Controller : SpawnedEnemies)
	{
		AliveCount += IsValid(Controller) && IsValid(Controller->GetPawn()) ? 1 : 0;
	}
	return AliveCount;
}

void UExtractionThreatDirectorComponent::AuditMatchCompletion()
{
	if (!MatchState || MatchState->GetSnapshot().MatchState != EExtractionMatchState::InRaid)
	{
		return;
	}
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	const bool bRaidExpired = GameState->GetServerWorldTimeSeconds() >= MatchState->GetSnapshot().RaidEndServerTime;
	bool bSawRun = false;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (PlayerState && !PlayerState->IsABot())
		{
			if (UExtractionRunStateComponent* Run =
				PlayerState->FindComponentByClass<UExtractionRunStateComponent>())
			{
				bSawRun = true;
				const EExtractionRunState State = Run->GetSnapshot().RunState;
				if (State == EExtractionRunState::InRaid || State == EExtractionRunState::Extracting)
				{
					if (!bRaidExpired)
					{
						return;
					}
					Run->MarkAbandoned();
				}
			}
		}
	}
	if (bSawRun)
	{
		if (bRaidExpired)
		{
			UE_LOG(LogExtractionThreatDirector, Log, TEXT("event=raid_expired match_id=%s"),
				*MatchState->GetSnapshot().MatchId);
		}
		MatchState->CompleteMatch();
	}
}
