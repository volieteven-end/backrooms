#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BRInventoryAssetTools.generated.h"
UCLASS()
class BACKROOMS_API UBRInventoryAssetTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="Backrooms|Editor") static bool BuildInventoryAsset();
};
