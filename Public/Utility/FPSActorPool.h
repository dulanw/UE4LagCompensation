// Copyright 2019 Dulan Wettasinghe. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FPSActorPool.generated.h"

class IFPSPooledActorInterface;

/**
 * 
 */
UCLASS()
class FPSGAME_API UFPSActorPool : public UObject
{
	GENERATED_BODY()
	
public:
	AActor* GetActorFromPool();

	/*Add object to pool, my be initially from this pool, this is basically adding it to the inactiveobjects pool
	 *@param InObject: the object to be added back to inactive pool from active pool
	*/
	bool AddActorToPool(AActor* InObject);

	/*@param InObjectPoolClass: class of the objects in the objectpool
	 *@param InPoolSize: intial size of the object pool
	 *@param InbExpandPool: allow the object pool to expand if more objects than the amount in poolsize are requested
	 */
	bool InitPool(UClass* InObjectPoolClass, int32 InPoolSize, bool InbExpandPool);


	/*@param InObjectPoolClass: class of the objects in the objectpool, the InTemplateActor will automatically be disabled and added to the inactive pool
	 *@param InPoolSize: intial size of the object pool
	 *@param InbExpandPool: allow the object pool to expand if more objects than the amount in poolsize are requested
	 */
	bool InitPoolFromActor(AActor* InTemplateActor, int32 InPoolSize, bool InbExpandPool);

public:
	/*A Queue of objects that contains the inactive objects*/
	UPROPERTY()
	TArray<AActor*> InactiveObjects;

	/*A Queue of objects that contains the active objects*/
	UPROPERTY()
	TSet<AActor*> ActiveObjects;

	/*TSubclassOf<class IObjectPoolInterface>*/
	/*The class of the objects within the pool*/
	UClass*	ObjectPoolClass = NULL;
	uint32 PoolSize = 0;
	bool bExpandPool = true;

	UPROPERTY()
	AActor* TemplateActor = nullptr;

	/*A Queue of objects that contains the active objects*/
	UPROPERTY()
	TArray<AActor*> AllObjects;
};
