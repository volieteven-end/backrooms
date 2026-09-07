#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BRMenuAssetTools.generated.h"

UCLASS()
class BACKROOMS_API UBRMenuAssetTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Generates an editable Widget Blueprint; editor-only implementation, no runtime asset creation. */
    UFUNCTION(BlueprintCallable, Category="Backrooms|Editor") static bool BuildMenuAssets();
};
