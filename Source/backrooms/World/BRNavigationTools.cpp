#include "World/BRNavigationTools.h"

#include "Engine/Engine.h"
#include "NavigationSystem.h"

bool UBRNavigationTools::RebuildNavigation(UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	UNavigationSystemV1* NavigationSystem = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!NavigationSystem)
	{
		UE_LOG(LogTemp, Error, TEXT("BR_NAVIGATION_BUILD result=FAIL reason=no_navigation_system"));
		return false;
	}

#if WITH_EDITOR
	constexpr uint8 ScriptedLoadLocks = ENavigationBuildLock::NoUpdateInEditor
		| ENavigationBuildLock::InitialLock
		| ENavigationBuildLock::AsyncLoadLock;
	NavigationSystem->RemoveNavigationBuildLock(
		ScriptedLoadLocks,
		UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
#endif

	NavigationSystem->Build();
	const bool bBuilt = NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) != nullptr;
	UE_LOG(LogTemp, Display, TEXT("BR_NAVIGATION_BUILD result=%s nav_data=%s"),
		bBuilt ? TEXT("PASS") : TEXT("FAIL"), bBuilt ? TEXT("present") : TEXT("missing"));
	return bBuilt;
}
