# Backrooms Co-op Framework

This UE 5.8 project now contains a native C++ foundation for a 1–4 player cooperative horror-escape vertical slice.

## Implemented

- `ABRGameMode`, replicated `ABRGameState`, and replicated `ABRPlayerState`.
- First-person `ABRPlayerCharacter` with Enhanced Input hooks plus immediately usable keyboard/mouse fallbacks.
- Server-authoritative interaction through `UBRInteractionComponent` and `IBRInteractable`.
- Replicated objectives and all-active-player extraction gating.
- Replicated downed/revive state suitable for teammate rescue.
- LAN-first `UBRSessionSubsystem` with host, search, join-first, destroy, and travel operations.
- Server-owned entity base and AI perception controller for sight/hearing-driven Blueprint or Behavior Tree logic.
- Automation coverage for objective and extraction rules.
- Null Online Subsystem configuration for local multi-process testing.

## Blueprint setup

1. Create Blueprint children of `BRPlayerCharacter`, `BRObjectiveActor`, `BRExtractionZone`, and `BREntityCharacter`.
2. Assign the existing `/Game/Input/IMC_Default`, movement, look, and jump assets to the player Blueprint. Create Sprint, Crouch, and Interact Input Actions when desired; classic keyboard fallbacks already work.
3. Add objective actors and one extraction zone to a level. Objectives self-register on the server.
4. Create a lobby widget that retrieves `BRSessionSubsystem` from the Game Instance and calls Host, Find, Join First, or Destroy.
5. For AI, create a Blueprint child of `BREntityAIController` or attach a Behavior Tree to it. All AI decision-making stays on the server.

## Networking rules

- Clients request interactions; the server revalidates interface, distance, and objective state.
- Objective counts, level phase, player readiness/downed state, sprint state, and entity state replicate.
- The server alone performs AI perception, extraction checks, objective completion, and map travel.
- GAS is intentionally excluded from the MVP. Add it only when abilities, effects, attributes, and prediction justify the extra framework.

## Local test flow

1. Run the editor with `Number of Players = 2–4` and `Net Mode = Play As Listen Server`.
2. Place at least one objective and one extraction zone.
3. Complete all objectives on one client and confirm all clients see the same count.
4. Move every non-downed player into the extraction zone and confirm the server fires team extraction.
5. Run `Automation RunTests Backrooms.Framework` from Session Frontend or `UnrealEditor-Cmd`.
