// Copyright Extraction Ops. All Rights Reserved.

#include "ExtractionDebugHUDWidget.h"

#include "ExtractionDebugDataLibrary.h"

#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionDebugHUDWidget)

namespace ExtractionDebugHUD
{
	template <typename EnumType>
	FString GetEnumName(EnumType Value)
	{
		const UEnum* Enum = StaticEnum<EnumType>();
		return Enum != nullptr ? Enum->GetNameStringByValue(static_cast<int64>(Value)) : TEXT("Unknown");
	}
}

void UExtractionDebugHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshSnapshotText();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RefreshTimerHandle,
			this,
			&ThisClass::RefreshSnapshotText,
			RefreshIntervalSeconds,
			true);
	}
}

void UExtractionDebugHUDWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	Super::NativeDestruct();
}

void UExtractionDebugHUDWidget::RefreshSnapshotText()
{
	if (SnapshotText == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	const AActor* ObservedActor = PlayerController != nullptr ? PlayerController->GetPawn() : nullptr;
	FExtractionNetworkDebugSnapshot Snapshot;
	if (!UExtractionDebugDataLibrary::GetNetworkDebugSnapshot(
		this,
		ObservedActor,
		PlayerController,
		Snapshot))
	{
		SnapshotText->SetText(FText::FromString(TEXT("Extraction snapshot unavailable")));
		return;
	}

	const FString Text = FString::Printf(
		TEXT("%s | Authority=%s | Role=%s/%s\n")
		TEXT("PC=%s PS=%s | Experience=%s\n")
		TEXT("Match=%s Threat=%s Run=%s"),
		*Snapshot.NetMode,
		Snapshot.bHasAuthority ? TEXT("true") : TEXT("false"),
		*Snapshot.LocalRole,
		*Snapshot.RemoteRole,
		Snapshot.bHasPlayerController ? TEXT("valid") : TEXT("none"),
		Snapshot.bHasPlayerState ? TEXT("valid") : TEXT("none"),
		Snapshot.ExperienceId.IsEmpty() ? TEXT("not loaded") : *Snapshot.ExperienceId,
		Snapshot.bHasMatchState ? *ExtractionDebugHUD::GetEnumName(Snapshot.MatchState) : TEXT("missing"),
		Snapshot.bHasMatchState ? *ExtractionDebugHUD::GetEnumName(Snapshot.ThreatLevel) : TEXT("missing"),
		Snapshot.bHasRunState ? *ExtractionDebugHUD::GetEnumName(Snapshot.RunState) : TEXT("missing"));

	SnapshotText->SetText(FText::FromString(Text));
}
