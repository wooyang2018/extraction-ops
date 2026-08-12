// Copyright Extraction Ops. All Rights Reserved.

#include "UI/ExtractionResultWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ExtractionResultWidget)

void UExtractionResultWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	const APlayerController* Controller = GetOwningPlayer();
	LifecycleComponent = Controller && Controller->PlayerState
		? Controller->PlayerState->FindComponentByClass<UExtractionRaidLifecycleComponent>() : nullptr;
	if (LifecycleComponent)
	{
		LifecycleComponent->OnResultChanged.AddUniqueDynamic(this, &ThisClass::HandleResultChanged);
		HandleResultChanged(LifecycleComponent->GetResultSnapshot());
	}
}

void UExtractionResultWidget::NativeDestruct()
{
	if (LifecycleComponent)
	{
		LifecycleComponent->OnResultChanged.RemoveDynamic(this, &ThisClass::HandleResultChanged);
	}
	Super::NativeDestruct();
}

void UExtractionResultWidget::HandleResultChanged(const FExtractionRunResultSnapshot& Result)
{
	if (!ResultText || !Result.IsTerminal())
	{
		return;
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ResultText->SetText(FText::Format(NSLOCTEXT("ExtractionOps", "RunResult",
		"Result {0}\nMatch {1}\nRun {2}\nItems {3}"),
		FText::AsNumber(static_cast<int32>(Result.Result)), FText::FromString(Result.MatchId),
		FText::FromString(Result.RunId), FText::AsNumber(Result.Items.Num())));
}
