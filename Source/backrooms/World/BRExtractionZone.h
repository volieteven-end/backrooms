#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRExtractionZone.generated.h"

class UBoxComponent;
class ABRGarageExitDoor;

UCLASS()
class BACKROOMS_API ABRExtractionZone : public AActor
{
	GENERATED_BODY()

public:
	ABRExtractionZone();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|Extraction")
	void CheckExtractionState();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backrooms|Extraction")
	FName NextMapName = NAME_None;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Backrooms|Extraction") TObjectPtr<ABRGarageExitDoor> RequiredExitDoor;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Backrooms|Extraction") bool bRequireAllPlayers=true;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Extraction")
	TObjectPtr<UBoxComponent> Zone;

	UFUNCTION()
	void OnZoneBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnZoneEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Backrooms|Extraction")
	void OnTeamReadyToExtract();

private:
	TSet<TWeakObjectPtr<APawn>> OverlappingPlayers;
	FTimerHandle ExtractionCheckTimer;
	bool bTriggered = false;
};
