// Copyright Winyunq, 2025. All Rights Reserved.

#include "MassLocationChangedObserver.h"
#include "FogOfWarMassBinding.h"
#include "MassCommonFragments.h"
#include "MassFogOfWarFragments.h"
#include "MassFogOfWarProcessors.h"
#include "MassExecutionContext.h"
#include "Subsystems/MinimapDataSubsystem.h"

UMassLocationChangedObserver::UMassLocationChangedObserver()
	: EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	ExecutionOrder.ExecuteAfter.Add(UInitialVisionProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UVisionProcessor::StaticClass()->GetFName());
}

void UMassLocationChangedObserver::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FOW_LOCATION_FRAGMENT>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassVisionFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassPreviousVisionFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FMassVisionEntityTag>(EMassFragmentPresence::All);
	EntityQuery.AddSubsystemRequirement<UMinimapDataSubsystem>(EMassFragmentAccess::ReadOnly);
	ProcessorRequirements.AddSubsystemRequirement<UMinimapDataSubsystem>(EMassFragmentAccess::ReadOnly);
}

void UMassLocationChangedObserver::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const UMinimapDataSubsystem* MinimapSubsystem = Context.GetSubsystem<UMinimapDataSubsystem>();
	if (!MinimapSubsystem)
	{
		return;
	}

	const bool bForceVisionUpdate = MinimapSubsystem->bDebugStressTestIgnoreCache;

	if (!bForceVisionUpdate)
	{
		if (!MinimapSubsystem->bVisionGridActive || !MinimapSubsystem->IsVisionGridReady())
		{
			return;
		}
	}

	const float MovementThresholdSq = FMath::Square(MinimapSubsystem->VisionUpdateWorldDistanceThreshold);
	const bool bUseMovementThreshold = MovementThresholdSq > 0.0f;
	const float VisionTileSize = MinimapSubsystem->VisionTileSize;

	EntityQuery.ForEachEntityChunk(Context, [bForceVisionUpdate, bUseMovementThreshold, MovementThresholdSq, VisionTileSize](FMassExecutionContext& Context)
	{
		const TConstArrayView<FOW_LOCATION_FRAGMENT> LocationList = Context.GetFragmentView<FOW_LOCATION_FRAGMENT>();
		const TConstArrayView<FMassVisionFragment> VisionList = Context.GetFragmentView<FMassVisionFragment>();
		const TConstArrayView<FMassPreviousVisionFragment> PreviousVisionList = Context.GetFragmentView<FMassPreviousVisionFragment>();
		const TArrayView<const FMassEntityHandle> Entities = Context.GetEntities();

		for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
		{
			bool bNeedsVisionUpdate = bForceVisionUpdate;
			const FMassPreviousVisionFragment& PreviousVision = PreviousVisionList[EntityIndex];

			if (!bNeedsVisionUpdate)
			{
				bNeedsVisionUpdate = !PreviousVision.PreviousVisionData.HasCachedData();
			}

			if (!bNeedsVisionUpdate)
			{
				const float SightRadius = VisionList[EntityIndex].SightRadius;
				const FVector CurrentLocation = FOW_GET_LOCATION(LocationList[EntityIndex]);
				const float GridSpaceRadius = SightRadius / VisionTileSize;
				const FVector2f OriginGridLocation = UMinimapDataSubsystem::ConvertWorldSpaceLocationToVisionGridSpace_Static(FVector2D(CurrentLocation));
				const FIntPoint OriginGlobalIJ = UMinimapDataSubsystem::ConvertVisionGridLocationToTileIJ_Static(OriginGridLocation);
				const FIntPoint CurrentMinIJ = UMinimapDataSubsystem::ConvertVisionGridLocationToTileIJ_Static(OriginGridLocation - GridSpaceRadius);

				if (!UMinimapDataSubsystem::IsVisionGridIJValid_Static(OriginGlobalIJ))
				{
					bNeedsVisionUpdate = true;
				}
				else
				{
					const int32 CurrentOriginIndex = UMinimapDataSubsystem::GetVisionGridGlobalIndex_Static(OriginGlobalIJ);
					bNeedsVisionUpdate =
						CurrentOriginIndex != PreviousVision.PreviousVisionData.CachedOriginGlobalIndex ||
						CurrentMinIJ != PreviousVision.PreviousVisionData.LocalAreaCachedMinIJ ||
						(bUseMovementThreshold && FVector::DistSquared(CurrentLocation, PreviousVision.PreviousVisionData.CachedOriginWorldLocation) > MovementThresholdSq);
				}
			}

			if (bNeedsVisionUpdate)
			{
				Context.Defer().AddTag<FMassLocationChangedTag>(Entities[EntityIndex]);
			}
		}
	});
}
