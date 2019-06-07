// Copyright 2019 Dulan Wettasinghe. All Rights Reserved.

#include "Utility/FPSActorPool.h"
#include "GameFramework/Actor.h"
#include "Utility/FPSPooledActorInterface.h"

AActor* UFPSActorPool::GetActorFromPool()
{
	if (!ObjectPoolClass)
		return nullptr;

	if (InactiveObjects.Num() > 0)
	{
		AActor* newobject = InactiveObjects.Pop(true);
		ActiveObjects.Add(newobject);

		return newobject;
	}

	if (!bExpandPool)
		return nullptr;

	UWorld* MyWorld = GetWorld();

	if (!MyWorld)
		return nullptr;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (TemplateActor)
		SpawnParams.Template = TemplateActor;

	AActor* newObject = MyWorld->SpawnActor<AActor>(ObjectPoolClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (newObject)
	{
		newObject->SetActorHiddenInGame(true);
		newObject->SetActorTickEnabled(false);
		newObject->SetActorEnableCollision(false);

		InactiveObjects.Add(newObject);
		AllObjects.Add(newObject);

		IFPSPooledActorInterface* newInterface = Cast<IFPSPooledActorInterface>(newObject);
		if (newInterface)
			newInterface->InitPooledActor(this);

		return newObject;
	}

	return nullptr;

}

bool UFPSActorPool::AddActorToPool(AActor* InObject)
{
	if (!InObject || ActiveObjects.Num() <= 0)
		return false;

	auto newPointer = ActiveObjects.Find(InObject);
	if (newPointer && ActiveObjects.Remove(InObject))
	{
		InactiveObjects.Add(InObject);
		return true;
	}
	else
	{
		return false;
	}

}

bool UFPSActorPool::InitPool(UClass* InObjectPoolClass, int32 InPoolSize, bool InbExpandPool)
{
	//#TODO maybe delete the extra ones
	if (InactiveObjects.Num() >= InPoolSize)
		return true;

	ObjectPoolClass = InObjectPoolClass;
	bExpandPool = InbExpandPool;
	PoolSize = InPoolSize;

	UWorld* MyWorld = GetWorld();

	if (!MyWorld)
		return false;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (TemplateActor)
		SpawnParams.Template = TemplateActor;

	for (int32 i = InactiveObjects.Num(); i < InPoolSize; i++)
	{
		AActor* newObject = MyWorld->SpawnActor<AActor>(ObjectPoolClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (newObject)
		{
			newObject->SetActorHiddenInGame(true);
			newObject->SetActorTickEnabled(false);
			newObject->SetActorEnableCollision(false);

			InactiveObjects.Add(newObject);
			AllObjects.Add(newObject);

			IFPSPooledActorInterface* newInterface = Cast<IFPSPooledActorInterface>(newObject);
			if (newInterface)
				newInterface->InitPooledActor(this);
		}
		else
			return false;
	}

	return true;
}

bool UFPSActorPool::InitPoolFromActor(AActor* InTemplateActor, int32 InPoolSize, bool InbExpandPool)
{
	/*return if we have already called InitPool once before*/
	if (!InTemplateActor)
		return false;

	//#TODO maybe delete the extra ones
	if (InactiveObjects.Num() >= InPoolSize)
		return true;

	ObjectPoolClass = InTemplateActor->GetClass();
	bExpandPool = InbExpandPool;
	PoolSize = InPoolSize;
	TemplateActor = InTemplateActor;


	UWorld* MyWorld = GetWorld();

	if (!MyWorld)
		return false;

	InactiveObjects.Add(InTemplateActor);
	InTemplateActor->SetActorHiddenInGame(true);
	InTemplateActor->SetActorTickEnabled(false);
	InTemplateActor->SetActorEnableCollision(false);

	return InitPool(ObjectPoolClass, InPoolSize, InbExpandPool);
}

