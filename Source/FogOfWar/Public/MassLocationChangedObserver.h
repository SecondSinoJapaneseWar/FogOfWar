// Copyright Winyunq, 2025. All Rights Reserved.

#pragma once

#include "MassProcessor.h"
#include "MassLocationChangedObserver.generated.h"

/**
 * Observes FogOfWar's bound location fragment and adds FMassLocationChangedTag
 * only when the current vision cache is missing or no longer matches the
 * current fog-grid position. Debug force-update can still mark every vision
 * provider each frame.
 */
UCLASS()
class FOGOFWAR_API UMassLocationChangedObserver : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassLocationChangedObserver();

protected:
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;

private:
	FMassEntityQuery EntityQuery;
};
