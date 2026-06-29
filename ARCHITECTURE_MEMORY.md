# MassBattle / FogOfWar Architecture Memory

This file records integration decisions and pitfalls that should survive across sessions.

## Confirmed MassBattle Facts

- MassBattleFrame agents do not use `FTransformFragment` as their authoritative runtime location.
- The authoritative MassBattle location fragment is `FLocating` in `Plugins/MassBattleFrame/Source/MassBattle/Public/Fragments/Transform.h`.
- `FLocating` carries `Location`, `PreLocation`, and `InitialLocation`. It is updated by MassBattle processors and components.
- FogOfWar must read `FLocating` by default. Reading `FTransformFragment` silently misses normal MassBattle agents.

## FogOfWar Binding Rule

- FogOfWar uses `FogOfWarMassBinding.h` as the single mapping point for Mass data.
- Default project binding:
  - `FOW_LOCATION_FRAGMENT` -> `FLocating`
  - `FOW_TEAM_FRAGMENT` -> `FTeam`
  - `FOW_GET_LOCATION(Fragment)` -> `Fragment.Location`
  - `FOW_GET_TEAM_INDEX(Fragment)` -> `Fragment.index`
- Fallback fragments exist inside FogOfWar for non-MassBattle builds:
  - `FFogOfWarLocationFragment`
  - `FFogOfWarTeamFragment`
- Do not scatter direct references to `FLocating`, `FTeam`, or `FTransformFragment` through FogOfWar processors. Route them through the binding header.

## Vision Update Pitfalls

- `UMassLocationChangedObserver` must not blindly mark all vision providers every frame in normal mode.
- Debug force update is configured on `AFogOfWar` for editor convenience, but Mass processors must read the synced value from `UMinimapDataSubsystem::bDebugStressTestIgnoreCache`.
- Normal mode should use FogOfWar's cached `FMassPreviousVisionFragment::PreviousVisionData` to decide whether an entity needs a vision refresh.
- The observer intentionally avoids relying on MassBattle `FLocating::PreLocation` timing. MassBattle updates `PreLocation` inside its own movement/grid registration flow, so cross-plugin ordering can make it a poor external change detector.
- `AFogOfWar::VisionUpdateWorldDistanceThreshold` is optional and is synced into `UMinimapDataSubsystem::VisionUpdateWorldDistanceThreshold`. `0` means grid/cache boundary comparison only.
- Plain MassBattle agents do not pass through `UMassVisionTrait`, so FogOfWar must auto-bind them. `UMassBattleFogOfWarBootstrapProcessor` is the bridge that adds `FMassVisionFragment`, `FMassPreviousVisionFragment`, and `FMassMinimapRepresentationFragment` once per Battle agent.
- Initial vision must perform an actual reveal pass. A processor that only adds `FMassVisionInitializedTag` without calling the vision calculation leaves the map black until some later movement happens.
- Do not gate RTS fog updates on render/frustum/distance culling tags. Off-camera friendly units still need to reveal the strategic fog state.
- Do not fix Mass worker-thread crashes by leaving hot processors on GameThread. The correct boundary is:
  - `AFogOfWar` handles GameThread-only setup/render work: Volume bounds, LineTrace height scan, texture/MID/RT updates, and post-process submission.
  - Mass processors handle runtime vision logic from fragments plus `UMinimapDataSubsystem` data.
  - `UMinimapDataSubsystem` is declared with `TMassExternalSubsystemTraits<GameThreadOnly=false>` and processors must declare both query requirements and processor-level `ProcessorRequirements` when accessing the subsystem outside `ForEachEntityChunk`.
- Optional/plugin-style Mass processors must not use `GetSubsystemChecked` for FogOfWar/Minimap subsystems. Use `Context.GetSubsystem` or `Context.GetMutableSubsystem` and return when null, so a missing minimap/fog module does not crash the core MassBattle simulation.
- Mass processors must not call `UGameplayStatics`, `GetWorld()`, actor iteration, or `AFogOfWar` in their hot path. If a processor truly needs Actor/render access, it is a bridge processor and should be isolated from the per-entity loop.
- Current visibility counter updates are Mass-owned but serial within the processor because many entities can touch the same tile. Do not switch this to chunk-parallel until the shared `VisibilityCounter` write hazard is solved with a reduction pass or explicit atomics.

## Minimap Fog

- The minimap should be low-frequency and UI-driven or subsystem-driven. It does not need per-frame precision.
- The preferred data source is `UMassBattleHashGridSubsystem::AgentGrid`, not a duplicate FogOfWar unit list.
- `UMinimapWidget` is the place to expose user-facing minimap options such as texture resolution, update interval, max encoded units, and rendering material.
- Team filtering is not mandatory for the first minimap implementation, but visibility ownership will eventually require a team/alliance resolver.
- Grid size and origin should be centralized. The intended direction is configuration-driven linkage between MassBattle HashGrid, minimap bounds, and FogOfWar bounds.
- Team color should be resolved with `TeamColors[TeamId]` through `UMinimapDataSubsystem::GetTeamColor`, not with repeated if/switch chains in hot paths.
- Minimap tile indexing is `Index = X * ResolutionY + Y`. Keep writers and readers aligned with `ConvertMinimapTileIJToWorldLocation_Static`.
- UMG can initialize before `AFogOfWar::Activate()` syncs final grid bounds. Refresh minimap material grid parameters before drawing instead of assuming one-time initialization order is correct.

## Scene Fog

- The current scene fog implementation is CPU tile/DDA based and outputs a post-process visibility texture.
- Runtime CPU visibility data now lives in `UMinimapDataSubsystem::VisionTiles`; `AFogOfWar` is the rendering/setup adapter, not the authoritative runtime data owner.
- The desired long-term RTS scene fog is different: camera-visible scene fog should be GPU-driven, using visible allied/friendly units as reveal sources inside the camera region.
- The CPU tile grid remains useful for minimap/explored state and gameplay queries, but it should not be mistaken for the final high-detail scene fog model.

## Do Not Rebuild

- Do not create a second authoritative unit registry for FogOfWar.
- Do not traverse all Mass entities for minimap hot paths when MassBattle HashGrid already contains spatially organized agent data.
- Do not add/remove Mass tags as a high-frequency state mechanism unless it is genuinely structural; prefer cached fragments or MassAPI flags for high-frequency state.

## Plugin Descriptor Pitfalls

- If `FogOfWar.Build.cs` depends on modules from another plugin, `FogOfWar.uplugin` must also list that plugin in its `Plugins` array. Otherwise UBT can compile inside the project but the plugin is fragile when packaged, migrated, or enabled independently.
- Current explicit plugin dependencies: `MassBattle`, `MassGameplay`, `MassBattleMinimap`, `EnhancedInput`, and `OpenRTSCamera`.
- Default command tags used by UI assets/subsystems must exist in `Config/DefaultGameplayTags.ini`. Runtime code that calls `RequestGameplayTag` before native registration can still trigger editor ensures.
