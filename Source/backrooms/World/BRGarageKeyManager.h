#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRGarageKeyManager.generated.h"

class ABRGarageKeyPickup;
class ABRGarageKeySocket;
class ABRGarageExitDoor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRGarageKeysChanged, int32, CollectedKeys, int32, RequiredKeys);

UCLASS(Config=Game)
class BACKROOMS_API ABRGarageKeyManager : public AActor
{
	GENERATED_BODY()

public:
	ABRGarageKeyManager();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|Garage")
	void HandleKeyCollected(ABRGarageKeyPickup* Pickup, APawn* CollectingPawn);

	UFUNCTION(BlueprintPure, Category = "Backrooms|Garage")
	int32 GetCollectedKeys() const { return CollectedKeys; }

	UFUNCTION(BlueprintPure, Category = "Backrooms|Garage")
	int32 GetRequiredKeys() const { return ActiveKeyCount; }

    UFUNCTION(BlueprintPure, Category="Backrooms|Garage") int32 GetInsertedKeys() const { return InsertedKeys; }
    UFUNCTION(BlueprintPure, Category="Backrooms|Garage") bool UsesKeySockets() const { return KeySockets.Num()>0; }
    UFUNCTION(BlueprintPure, Category="Backrooms|Garage") bool AreAllKeysInserted() const;
    bool CanInsertKey(const ABRGarageKeySocket* Socket) const;
    bool TryInsertKey(ABRGarageKeySocket* Socket);
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Backrooms|Garage") TArray<TObjectPtr<ABRGarageKeySocket>> KeySockets;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Backrooms|Garage") TObjectPtr<ABRGarageExitDoor> ExitDoor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage", meta = (ClampMin = "1"))
	int32 KeysRequired = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage", meta = (ClampMin = "0"))
	int32 UnlockableDoorCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage")
	int32 RandomSeed = 0;

    /** Extra pickups for quick escape verification; normal loot and the four-key goal are unchanged. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Backrooms|Garage|Testing")
    bool bSpawnPointTestKeys = false;

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Garage")
	FBRGarageKeysChanged OnGarageKeysChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_KeyProgress, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage")
	int32 ActiveKeyCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_KeyProgress, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage")
	int32 CollectedKeys = 0;
    UPROPERTY(ReplicatedUsing=OnRep_KeyProgress, VisibleAnywhere, BlueprintReadOnly, Category="Backrooms|Garage") int32 InsertedKeys=0;

	UFUNCTION()
	void OnRep_KeyProgress();

private:
    void SpawnPointTestKeys();
    bool ConfigureCabinetLoot(FRandomStream& RandomStream);
	void ConfigureKeyCandidates(FRandomStream& RandomStream);
	void ConfigureGarageDoors(FRandomStream& RandomStream);
	TSet<TWeakObjectPtr<ABRGarageKeyPickup>> CountedPickups;
};
