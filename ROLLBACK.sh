#!/usr/bin/env bash
set -euo pipefail

root="${1:-/mnt/e/UNREAL/ue projects/backrooms}"
baseline="${2:-$root/Saved/CodexFrameworkBaseline}"

if [[ ! -f "$root/backrooms.uproject" || ! -f "$baseline/backrooms.uproject" ]]; then
  echo "ROLLBACK_ERROR invalid project or baseline path" >&2
  exit 2
fi

created_files=(
  "Source/backrooms/Core/BRFrameworkTypes.h"
  "Source/backrooms/Core/BRGameplayRules.h"
  "Source/backrooms/Core/BRGameplayRules.cpp"
  "Source/backrooms/Core/BRGameState.h"
  "Source/backrooms/Core/BRGameState.cpp"
  "Source/backrooms/Core/BRGameMode.h"
  "Source/backrooms/Core/BRGameMode.cpp"
  "Source/backrooms/Interaction/BRInteractable.h"
  "Source/backrooms/Interaction/BRInteractionComponent.h"
  "Source/backrooms/Interaction/BRInteractionComponent.cpp"
  "Source/backrooms/Player/BRPlayerState.h"
  "Source/backrooms/Player/BRPlayerState.cpp"
  "Source/backrooms/Player/BRDownedComponent.h"
  "Source/backrooms/Player/BRDownedComponent.cpp"
  "Source/backrooms/Player/BRPlayerCharacter.h"
  "Source/backrooms/Player/BRPlayerCharacter.cpp"
  "Source/backrooms/World/BRObjectiveActor.h"
  "Source/backrooms/World/BRObjectiveActor.cpp"
  "Source/backrooms/World/BRExtractionZone.h"
  "Source/backrooms/World/BRExtractionZone.cpp"
  "Source/backrooms/AI/BREntityCharacter.h"
  "Source/backrooms/AI/BREntityCharacter.cpp"
  "Source/backrooms/AI/BREntityAIController.h"
  "Source/backrooms/AI/BREntityAIController.cpp"
  "Source/backrooms/Online/BRSessionSubsystem.h"
  "Source/backrooms/Online/BRSessionSubsystem.cpp"
  "Source/backrooms/Tests/BRFrameworkTests.cpp"
  "FRAMEWORK.md"
)

for relative in "${created_files[@]}"; do
  rm -f -- "$root/$relative"
done

cp -- "$baseline/backrooms.uproject" "$root/backrooms.uproject"
cp -R -- "$baseline/Source/." "$root/Source/"
cp -R -- "$baseline/Config/." "$root/Config/"

hash="$(sha256sum -- "$root/backrooms.uproject" | awk '{print $1}')"
echo "ROLLBACK_OK root=$root uproject_sha256=$hash created_files_removed=${#created_files[@]}"
