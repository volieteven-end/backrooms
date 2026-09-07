#include "Online/BRSessionSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"

void UBRSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UBRSessionSubsystem::Deinitialize()
{
	ClearDelegateHandles();
	SessionSearch.Reset();
	Super::Deinitialize();
}

IOnlineSessionPtr UBRSessionSubsystem::GetSessionInterface() const
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		return Subsystem->GetSessionInterface();
	}
	return nullptr;
}

void UBRSessionSubsystem::HostSession(const int32 MaxPlayers, const bool bLAN, const FString& LobbyMap)
{
	PendingMaxPlayers = FMath::Clamp(MaxPlayers, 1, 4);
	bPendingLAN = bLAN;
	PendingLobbyMap = LobbyMap;

	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnHostComplete.Broadcast(false, TEXT("Online session interface is unavailable."));
		return;
	}

	if (Sessions->GetNamedSession(NAME_GameSession))
	{
		bCreateAfterDestroy = true;
		DestroyCurrentSession();
		return;
	}

	CreateSessionNow();
}

void UBRSessionSubsystem::CreateSessionNow()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid()) return;

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = bPendingLAN;
	Settings.NumPublicConnections = PendingMaxPlayers;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(FName(TEXT("MAPNAME")), PendingLobbyMap, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UBRSessionSubsystem::HandleCreateSessionComplete));
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		CreateHandle.Reset();
		OnHostComplete.Broadcast(false, TEXT("CreateSession request was rejected."));
	}
}

void UBRSessionSubsystem::HandleCreateSessionComplete(FName SessionName, const bool bSuccess)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface(); Sessions.IsValid())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
	CreateHandle.Reset();
	OnHostComplete.Broadcast(bSuccess, bSuccess ? TEXT("Session created.") : TEXT("Session creation failed."));

	if (bSuccess && GetGameInstance())
	{
		GetGameInstance()->GetWorld()->ServerTravel(PendingLobbyMap + TEXT("?listen"));
	}
}

void UBRSessionSubsystem::FindSessions(const int32 MaxResults, const bool bLAN)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnSearchComplete.Broadcast(false, 0);
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = FMath::Max(1, MaxResults);
	SessionSearch->bIsLanQuery = bLAN;
	FindHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UBRSessionSubsystem::HandleFindSessionsComplete));
	if (!Sessions->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		FindHandle.Reset();
		OnSearchComplete.Broadcast(false, 0);
	}
}

void UBRSessionSubsystem::HandleFindSessionsComplete(const bool bSuccess)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface(); Sessions.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}
	FindHandle.Reset();
	OnSearchComplete.Broadcast(bSuccess, SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0);
}

void UBRSessionSubsystem::JoinFirstFoundSession()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid() || !SessionSearch.IsValid() || SessionSearch->SearchResults.IsEmpty())
	{
		OnJoinComplete.Broadcast(false, TEXT("No session search result is available."));
		return;
	}

	JoinHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UBRSessionSubsystem::HandleJoinSessionComplete));
	if (!Sessions->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[0]))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		JoinHandle.Reset();
		OnJoinComplete.Broadcast(false, TEXT("JoinSession request was rejected."));
	}
}

void UBRSessionSubsystem::HandleJoinSessionComplete(FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (Sessions.IsValid()) Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	JoinHandle.Reset();

	FString ConnectString;
	const bool bSuccess = Result == EOnJoinSessionCompleteResult::Success && Sessions.IsValid()
		&& Sessions->GetResolvedConnectString(SessionName, ConnectString);
	if (bSuccess && GetGameInstance())
	{
		if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
		{
			PC->ClientTravel(ConnectString, TRAVEL_Absolute);
		}
	}
	OnJoinComplete.Broadcast(bSuccess, bSuccess ? ConnectString : TEXT("Failed to resolve or join the session."));
}

void UBRSessionSubsystem::DestroyCurrentSession()
{
	IOnlineSessionPtr Sessions = GetSessionInterface();
	if (!Sessions.IsValid())
	{
		OnDestroyComplete.Broadcast(false, TEXT("Online session interface is unavailable."));
		return;
	}

	DestroyHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UBRSessionSubsystem::HandleDestroySessionComplete));
	if (!Sessions->DestroySession(NAME_GameSession))
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		DestroyHandle.Reset();
		OnDestroyComplete.Broadcast(false, TEXT("DestroySession request was rejected."));
	}
}

void UBRSessionSubsystem::HandleDestroySessionComplete(FName SessionName, const bool bSuccess)
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface(); Sessions.IsValid())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
	DestroyHandle.Reset();
	OnDestroyComplete.Broadcast(bSuccess, bSuccess ? TEXT("Session destroyed.") : TEXT("Session destruction failed."));
	if (bSuccess && bCreateAfterDestroy)
	{
		bCreateAfterDestroy = false;
		CreateSessionNow();
	}
}

void UBRSessionSubsystem::ClearDelegateHandles()
{
	if (IOnlineSessionPtr Sessions = GetSessionInterface(); Sessions.IsValid())
	{
		if (CreateHandle.IsValid()) Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		if (FindHandle.IsValid()) Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		if (JoinHandle.IsValid()) Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		if (DestroyHandle.IsValid()) Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
	CreateHandle.Reset();
	FindHandle.Reset();
	JoinHandle.Reset();
	DestroyHandle.Reset();
}
