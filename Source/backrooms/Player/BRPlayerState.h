#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BRPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBRPlayerStateChanged);

UCLASS()
class BACKROOMS_API ABRPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ABRPlayerState();
    UFUNCTION(BlueprintPure) FGuid GetRoomPlayerId() const { return RoomPlayerId; }
    UFUNCTION(BlueprintPure) bool IsRoomOwner() const { return bRoomOwner; }
    void SetRoomIdentity(const FGuid& Id, const FString& Nickname, bool bOwner, bool bNewReady);
    virtual void CopyProperties(APlayerState* Target) override;

	UFUNCTION(BlueprintPure, Category = "Backrooms|Player")
	bool IsReady() const { return bReady; }

	UFUNCTION(BlueprintPure, Category = "Backrooms|Player")
	bool IsDowned() const { return bDowned; }

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Backrooms|Player")
	void ServerSetReady(bool bNewReady);

	void SetDownedState(bool bNewDowned);

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Player")
	FBRPlayerStateChanged OnPlayerStateChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    UPROPERTY(ReplicatedUsing=OnRep_PlayerFlags) FGuid RoomPlayerId;
    UPROPERTY(ReplicatedUsing=OnRep_PlayerFlags) bool bRoomOwner = false;
	UPROPERTY(ReplicatedUsing = OnRep_PlayerFlags, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Player")
	bool bReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_PlayerFlags, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Player")
	bool bDowned = false;

	UFUNCTION()
	void OnRep_PlayerFlags();
};
