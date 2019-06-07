// Copyright 2019 Dulan Wettasinghe. All Rights Reserved.

#include "Utility/FPSPooledActorInterface.h"
#include "Utility/FPSActorPool.h"

// Add default functionality here for any IPooledActorInterface functions that are not pure virtual.

bool IFPSPooledActorInterface::InitPooledActor(UFPSActorPool* InPool)
{
	if (!InPool || ParentPool)
		return false;

	ParentPool = InPool;
	return true;
}

bool IFPSPooledActorInterface::DeActivatePooledActor()
{
	AActor* DeactvateActor = Cast<AActor>(this);
	if (ParentPool && DeactvateActor)
		return ParentPool->AddActorToPool(DeactvateActor);

	return false;
}
