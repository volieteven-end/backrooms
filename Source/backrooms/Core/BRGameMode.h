#pragma once

#include "CoreMinimal.h"
#include "Online/BRNetworkGameMode.h"
#include "BRGameMode.generated.h"

UCLASS()
class BACKROOMS_API ABRGameMode : public ABRNetworkGameMode
{
	GENERATED_BODY()

public:
	ABRGameMode();

	virtual void StartPlay() override;
    virtual void InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|Extraction")
	void HandleTeamExtracted(FName NextMapName);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Extraction")
	bool bAutoTravelOnExtraction = false;

private:
	TMap<TWeakObjectPtr<AController>, TWeakObjectPtr<AActor>> AssignedStarts;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Backrooms|Extraction")
	void OnTeamExtracted(FName NextMapName);
};
