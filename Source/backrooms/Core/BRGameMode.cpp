#include "Core/BRGameMode.h"

#include "Core/BRGameState.h"
#include "Online/BRRoomDirectorySubsystem.h"
#include "Engine/GameInstance.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRPlayerState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Tests/BRNetworkSmokeProbe.h"
#include "Tests/BRItemSmokeProbe.h"
#include "Tests/BRInventorySmokeProbe.h"
#include "Tests/BREntitySmokeProbe.h"
#include "Tests/BRStaminaSmokeProbe.h"
#include "Tests/BRKeyInsertionSmokeProbe.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ABRGameMode::ABRGameMode()
{
	DefaultPawnClass = ABRPlayerCharacter::StaticClass();
	GameStateClass = ABRGameState::StaticClass();
	PlayerStateClass = ABRPlayerState::StaticClass();
	bUseSeamlessTravel = true;
}

void ABRGameMode::StartPlay()
{
	Super::StartPlay();
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("BRKeyInsertionSmoke")))GetWorld()->SpawnActor<ABRKeyInsertionSmokeProbe>();
    if (GetNetMode()==NM_DedicatedServer && FParse::Param(FCommandLine::Get(),TEXT("BRStaminaSmoke"))) GetWorld()->SpawnActor<ABRStaminaSmokeProbe>();
    if (FParse::Param(FCommandLine::Get(), TEXT("BREntitySmoke"))) GetWorld()->SpawnActor<ABREntitySmokeProbe>();
    if(GetNetMode()!=NM_DedicatedServer && (FParse::Param(FCommandLine::Get(),TEXT("BRInventorySmoke")) || FParse::Param(FCommandLine::Get(),TEXT("BRArmsViews"))))GetWorld()->SpawnActor<ABRInventorySmokeProbe>();
    if(GetNetMode()!=NM_DedicatedServer && (FParse::Param(FCommandLine::Get(),TEXT("BRItemsSmoke")) || FParse::Param(FCommandLine::Get(),TEXT("BRPickupPromptSmoke"))))GetWorld()->SpawnActor<ABRItemSmokeProbe>();
    if (GetNetMode()==NM_DedicatedServer && FParse::Param(FCommandLine::Get(),TEXT("BRNetworkSmoke"))) GetWorld()->SpawnActor<ABRNetworkSmokeProbe>();
#endif
	if (ABRGameState* State = GetGameState<ABRGameState>())
	{
		State->SetLevelPhase(EBRLevelPhase::Exploring);
	}
}

AActor* ABRGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    if (auto* Assigned = AssignedStarts.Find(Player)) if (Assigned->IsValid()) return Assigned->Get();
    for (auto It = AssignedStarts.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
    TArray<APlayerStart*> Starts;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) Starts.Add(*It);
    Starts.Sort([](const APlayerStart& A, const APlayerStart& B) { return A.GetName() < B.GetName(); });
    for (APlayerStart* Start : Starts)
    {
        bool bUsed = false;
        for (const auto& Pair : AssignedStarts) if (Pair.Value.Get() == Start) bUsed = true;
        if (!bUsed)
        {
            AssignedStarts.Add(Player, Start);
            UE_LOG(LogTemp, Display, TEXT("BR_PLAYER_START result=ASSIGNED name=%s location=%s"), *Start->GetName(), *Start->GetActorLocation().ToCompactString());
            return Start;
        }
    }
    return Super::ChoosePlayerStart_Implementation(Player);
}

void ABRGameMode::HandleTeamExtracted(const FName NextMapName)
{
	if (!HasAuthority())
	{
		return;
	}

	if (ABRGameState* State = GetGameState<ABRGameState>())
	{
        if(State->GetLevelPhase()==EBRLevelPhase::Completed || State->GetLevelPhase()==EBRLevelPhase::Failed)return;
		State->SetLevelPhase(GetNetMode()==NM_DedicatedServer?EBRLevelPhase::Escaping:EBRLevelPhase::Completed);
	}

	OnTeamExtracted(NextMapName);
	if (GetNetMode() == NM_DedicatedServer)
	{
		GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->FinishRound(true);
		return;
	}
	if (bAutoTravelOnExtraction && !NextMapName.IsNone() && GetWorld())
	{
		GetWorld()->ServerTravel(NextMapName.ToString());
	}
}

void ABRGameMode::InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage)
{
    Super::InitGame(MapName,Options,ErrorMessage);
    if(DefaultPawnClass==ABRPlayerCharacter::StaticClass())
        if(auto* EditablePawn=LoadClass<ABRPlayerCharacter>(nullptr,TEXT("/Game/Gameplay/Player/BP_BRPlayerCharacter.BP_BRPlayerCharacter_C")))DefaultPawnClass=EditablePawn;
}
