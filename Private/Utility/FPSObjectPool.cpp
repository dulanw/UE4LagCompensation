// Copyright 2019 Dulan Wettasinghe. All Rights Reserved.

#include "Utility/FPSObjectPool.h"

UObject* UFPSObjectPool::GetObject()
{
	if (!ObjectPoolClass)
		return nullptr;

	if (InActiveObjects.Num() > 0)
	{
		auto newobject = InActiveObjects.Pop();
		ActiveObjects.Add(newobject);

		return newobject;
	}
	else
	{
		if (bExpandPool)
		{
			UObject* newObject = NewObject<UObject>(this, ObjectPoolClass);
			if (newObject)
			{
				InActiveObjects.Add(newObject);
				return newObject;
			}
			else
				return nullptr;
		}
		else
		{
			return nullptr;
		}
			
	}
}

bool UFPSObjectPool::AddObject(UObject* InObject)
{
	auto newPointer = ActiveObjects.Find(InObject);
	if (newPointer)
	{
		ActiveObjects.Remove(InObject);
		InActiveObjects.Add(InObject);
		return true;
	}
	else
	{
		return false;
	}

}

bool UFPSObjectPool::InitPool(UClass* InObjectPoolClass, uint32 InPoolSize, bool InbExpandPool)
{
	/*return if we have already called InitPool once before*/
	if (ObjectPoolClass)
		return false;
	
	ObjectPoolClass = InObjectPoolClass;
	bExpandPool = InbExpandPool;
	for (uint32 i = 0; i < InPoolSize; i++)
	{
		UObject* newObject = NewObject<UObject>(this, ObjectPoolClass);

		if (newObject)
		{
			InActiveObjects.Add(newObject);
		}
		else
			return false;
	}

	return true;
}

