// Copyright Extraction Ops. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "ExtractionDebugDataLibrary.h"
#include "ExtractionOpsTypes.h"
#include "AI/ExtractionThreatDirectorComponent.h"
#include "AbilitySystem/Executions/ExtractionDamageExecution.h"
#include "Inventory/ExtractionInventoryComponent.h"
#include "Network/ExtractionNetworkValidation.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionDebugSnapshotInvalidContextTest,
	"ExtractionOps.DebugSnapshot.InvalidContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionDebugSnapshotInvalidContextTest::RunTest(const FString& Parameters)
{
	FExtractionNetworkDebugSnapshot Snapshot;
	Snapshot.NetMode = TEXT("Stale");
	Snapshot.bHasAuthority = true;

	TestFalse(TEXT("A null world context cannot produce a snapshot"),
		UExtractionDebugDataLibrary::GetNetworkDebugSnapshot(nullptr, nullptr, nullptr, Snapshot));
	TestTrue(TEXT("Failure resets the output snapshot"), Snapshot.NetMode.IsEmpty());
	TestFalse(TEXT("Failure cannot preserve stale authority"), Snapshot.bHasAuthority);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionMatchStateRulesTest,
	"ExtractionOps.StateRules.Match",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionMatchStateRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Loading can enter raid"), FExtractionStateRules::CanTransitionMatch(
		EExtractionMatchState::Loading, EExtractionMatchState::InRaid));
	TestTrue(TEXT("Raid can complete"), FExtractionStateRules::CanTransitionMatch(
		EExtractionMatchState::InRaid, EExtractionMatchState::Completed));
	TestFalse(TEXT("Completed is terminal"), FExtractionStateRules::CanTransitionMatch(
		EExtractionMatchState::Completed, EExtractionMatchState::InRaid));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionRunStateRulesTest,
	"ExtractionOps.StateRules.Run",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionRunStateRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Raid can start extraction"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::InRaid, EExtractionRunState::Extracting));
	TestTrue(TEXT("Extraction can be cancelled"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::Extracting, EExtractionRunState::InRaid));
	TestTrue(TEXT("Extraction can succeed"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::Extracting, EExtractionRunState::Extracted));
	TestFalse(TEXT("Extracted is terminal"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::Extracted, EExtractionRunState::InRaid));
	TestFalse(TEXT("Death cannot become extraction"), FExtractionStateRules::CanTransitionRun(
		EExtractionRunState::Dead, EExtractionRunState::Extracting));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionThreatRulesTest,
	"ExtractionOps.StateRules.Threat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionThreatRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("No terminal means no threat"),
		FExtractionStateRules::GetThreatLevelForTerminalCount(0), EExtractionThreatLevel::None);
	TestEqual(TEXT("First terminal sets low threat"),
		FExtractionStateRules::GetThreatLevelForTerminalCount(1), EExtractionThreatLevel::Low);
	TestEqual(TEXT("Second terminal sets high threat"),
		FExtractionStateRules::GetThreatLevelForTerminalCount(2), EExtractionThreatLevel::High);
	TestEqual(TEXT("Third terminal sets critical threat"),
		FExtractionStateRules::GetThreatLevelForTerminalCount(3), EExtractionThreatLevel::Critical);
	TestEqual(TEXT("Critical reward is capped"),
		FExtractionStateRules::GetRewardMultiplier(EExtractionThreatLevel::Critical), 2.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionZoneStateRulesTest,
	"ExtractionOps.StateRules.Zone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionZoneStateRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Terminal unlocks the selected zone"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Locked, EExtractionZoneState::Available));
	TestTrue(TEXT("An available zone can start a countdown"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Available, EExtractionZoneState::Countdown));
	TestTrue(TEXT("A countdown can succeed"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Countdown, EExtractionZoneState::Extracted));
	TestTrue(TEXT("A cancelled countdown can accept another player"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Cancelled, EExtractionZoneState::Available));
	TestFalse(TEXT("A locked zone cannot skip the unlock requirement"), FExtractionStateRules::CanTransitionZone(
		EExtractionZoneState::Locked, EExtractionZoneState::Countdown));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionArmorDamageTest,
	"ExtractionOps.GAS.ArmorDamageSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionArmorDamageTest::RunTest(const FString& Parameters)
{
	const FExtractionDamageSplit First = FExtractionDamageRules::SplitDamage(20.0f, 30.0f);
	TestEqual(TEXT("20 damage is fully absorbed"), First.ArmorDamage, 20.0f);
	TestEqual(TEXT("Health remains untouched"), First.HealthDamage, 0.0f);
	const FExtractionDamageSplit Second = FExtractionDamageRules::SplitDamage(25.0f, 10.0f);
	TestEqual(TEXT("Remaining armor is consumed"), Second.ArmorDamage, 10.0f);
	TestEqual(TEXT("Overflow reaches health"), Second.HealthDamage, 15.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionInventoryRulesTest,
	"ExtractionOps.Inventory.CommandRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionInventoryRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Slot zero is valid"), FExtractionInventoryRules::IsSlotIndexValid(0));
	TestTrue(TEXT("Slot eleven is valid"), FExtractionInventoryRules::IsSlotIndexValid(11));
	TestFalse(TEXT("Slot twelve is outside capacity"), FExtractionInventoryRules::IsSlotIndexValid(12));
	TestTrue(TEXT("A strict subset can be split"), FExtractionInventoryRules::CanSplitStack(5, 2));
	TestFalse(TEXT("The full stack cannot be split away"), FExtractionInventoryRules::CanSplitStack(5, 5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionThreatScheduleTest,
	"ExtractionOps.AI.ThreatSchedule",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionThreatScheduleTest::RunTest(const FString& Parameters)
{
	const FExtractionThreatWaveDecision Low = FExtractionThreatScheduleRules::GetWave(EExtractionThreatLevel::Low);
	const FExtractionThreatWaveDecision High = FExtractionThreatScheduleRules::GetWave(EExtractionThreatLevel::High);
	const FExtractionThreatWaveDecision Critical = FExtractionThreatScheduleRules::GetWave(EExtractionThreatLevel::Critical);
	TestEqual(TEXT("Low spawns patrol shooters"), Low.Role, EExtractionEnemyRole::PatrolShooter);
	TestEqual(TEXT("High spawns flankers"), High.Role, EExtractionEnemyRole::Flanker);
	TestEqual(TEXT("Critical spawns one elite"), Critical.Count, 1);
	TestTrue(TEXT("Threat levels use distinct wave bits"),
		FExtractionThreatScheduleRules::GetWaveBit(EExtractionThreatLevel::Low)
		!= FExtractionThreatScheduleRules::GetWaveBit(EExtractionThreatLevel::High));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FExtractionNetworkValidationTest,
	"ExtractionOps.Network.ShotValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FExtractionNetworkValidationTest::RunTest(const FString& Parameters)
{
	FExtractionShotValidationInput Input;
	Input.bOwnedByRequestingConnection = true;
	Input.bWeaponEquipped = true;
	Input.AuthoritativeAmmo = 1;
	Input.SecondsSinceLastAcceptedShot = 1.0;
	Input.MinimumFireInterval = 0.12;
	Input.RequestedTraceDistance = 1000.0f;
	TestEqual(TEXT("Legal intent passes"), FExtractionNetworkValidation::ValidateShot(Input),
		EExtractionShotRejectionReason::None);
	Input.bDuplicateRequest = true;
	TestEqual(TEXT("Duplicate intent is rejected"), FExtractionNetworkValidation::ValidateShot(Input),
		EExtractionShotRejectionReason::DuplicateRequest);
	Input.bDuplicateRequest = false;
	Input.AuthoritativeAmmo = 0;
	TestEqual(TEXT("Server ammo is authoritative"), FExtractionNetworkValidation::ValidateShot(Input),
		EExtractionShotRejectionReason::NoAmmo);
	return true;
}

#endif
