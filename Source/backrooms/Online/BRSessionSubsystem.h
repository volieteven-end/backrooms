#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "BRSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRSessionOperationComplete, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRSessionSearchComplete, bool, bSuccess, int32, ResultCount);

UCLASS()
class BACKROOMS_API UBRSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Backrooms|Online")
	void HostSession(int32 MaxPlayers = 4, bool bLAN = true, const FString& LobbyMap = TEXT("/Game/FirstPerson/Lvl_FirstPerson"));

	UFUNCTION(BlueprintCallable, Category = "Backrooms|Online")
	void FindSessions(int32 MaxResults = 20, bool bLAN = true);

	UFUNCTION(BlueprintCallable, Category = "Backrooms|Online")
	void JoinFirstFoundSession();

	UFUNCTION(BlueprintCallable, Category = "Backrooms|Online")
	void DestroyCurrentSession();

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Online")
	FBRSessionOperationComplete OnHostComplete;

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Online")
	FBRSessionSearchComplete OnSearchComplete;

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Online")
	FBRSessionOperationComplete OnJoinComplete;

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Online")
	FBRSessionOperationComplete OnDestroyComplete;

private:
	IOnlineSessionPtr GetSessionInterface() const;
	void CreateSessionNow();
	void HandleCreateSessionComplete(FName SessionName, bool bSuccess);
	void HandleFindSessionsComplete(bool bSuccess);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bSuccess);
	void ClearDelegateHandles();

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;
	FString PendingLobbyMap;
	int32 PendingMaxPlayers = 4;
	bool bPendingLAN = true;
	bool bCreateAfterDestroy = false;
};
