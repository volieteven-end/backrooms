#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BRNavigationTools.generated.h"

UCLASS()
class BACKROOMS_API UBRNavigationTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Synchronously rebuilds navigation after scripted editor map reconstruction. */
	UFUNCTION(BlueprintCallable, Category = "Backrooms|Navigation", meta = (WorldContext = "WorldContextObject"))
	static bool RebuildNavigation(UObject* WorldContextObject);
};
