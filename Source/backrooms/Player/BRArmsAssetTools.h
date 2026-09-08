#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BRArmsAssetTools.generated.h"

UCLASS()
class BACKROOMS_API UBRArmsAssetTools : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Used by Tools/Animation/Build-RelaxedArms.py; asset authoring is editor only.
    UFUNCTION(BlueprintCallable, Category="Backrooms|Editor")
    static bool BuildLocomotionBlendSpace();
};
