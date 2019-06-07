// Copyright 2019 Dulan Wettasinghe. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Set.h"
#include "UObject/NoExportTypes.h"
#include "FPSObjectPool.generated.h"

/**
 * This is using 2 different objects lists, so if we have 1000 objects, we won't be using a for loop to look for an inactive object
 * Need to implement PoolObjectInterface in whatever class is being used with this objectpool
 */
UCLASS()
class FPSGAME_API UFPSObjectPool : public UObject
{
	GENERATED_BODY()

public:
	UObject* GetObject();

	/*Add object to pool, my be initially from this pool, this is basically adding it to the inactiveobjects pool
	 *@param InObject: the object to be added back to inactive pool from active pool
	*/
	bool AddObject(UObject* InObject);

	/*@param InObjectPoolClass: class of the objects in the objectpool
	 *@param InPoolSize: intial size of the object pool
	 *@param InbExpandPool: allow the object pool to expand if more objects than the amount in poolsize are requested
	 */
	bool InitPool(UClass* InObjectPoolClass, uint32 InPoolSize, bool InbExpandPool);

private:
	/*TSubclassOf<class IObjectPoolInterface>*/
	/*The class of the objects within the pool*/
	UClass*	ObjectPoolClass = NULL;
	uint32 PoolSize = 0;
	bool bExpandPool = true;

	/*A Queue of objects that contains the inactive objects*/
	UPROPERTY(Transient)
	TArray<UObject*> InActiveObjects;

	/*A Queue of objects that contains the active objects*/
	UPROPERTY(Transient)
	TSet<UObject*> ActiveObjects;
};
