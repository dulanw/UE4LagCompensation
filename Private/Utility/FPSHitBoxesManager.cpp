// Copyright 2019 Dulan Wettasinghe. All Rights Reserved.

#include "Utility/FPSHitBoxesManager.h"
#include "Utility/FPSActorPool.h"
#include "Utility/FPSHitBoxesActor.h"
#include "ConfigCacheIni.h"
#include "Components/ShapeComponent.h"

// Sets default values for this component's properties
UFPSHitBoxesManager::UFPSHitBoxesManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	CompensationTime = 150.0f;
	// ...
}


// Called when the game starts
void UFPSHitBoxesManager::BeginPlay()
{
	Super::BeginPlay();
	
	/*This should only happen on the server, both listen and dedicated*/
	if (GetOwner()->Role == ENetRole::ROLE_Authority)
	{
		HitBoxPool = NewObject<UFPSActorPool>(this, TEXT("HitBoxPool"));
		if (HitBoxPool)
		{
			/*FPS of the server or listen server, use default 128 for listen servers*/
			/*assuming that the client is running at 32 fps. will scale up in tick component*/
			int32 UpdateRate = 64;
			if (IsRunningDedicatedServer())
			{
				int32 ReadUpdateRate;
				if (GConfig->GetInt(TEXT("/Script/OnlineSubsystemUtils.IpNetDriver"), TEXT("NetServerMaxTickRate"), ReadUpdateRate, GEngineIni))
					UpdateRate = ReadUpdateRate;
			}

			HitBoxLifeTime = CompensationTime / 1000.0f;
			/*how many instances we should create on begin play*/
			int32 NumInstances = UpdateRate * HitBoxLifeTime;

			HitBoxPool->InitPool(AFPSHitBoxesActor::StaticClass(), NumInstances, true);
			UE_LOG(LogTemp, Warning, TEXT("%d"), NumInstances);
		}
	}
}


// Called every frame
void UFPSHitBoxesManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner()->Role == ENetRole::ROLE_Authority)
	{
		AFPSHitBoxesActor* NewHitBox = Cast<AFPSHitBoxesActor>(HitBoxPool->GetActorFromPool());
		if (ensure(NewHitBox != NULL))
		{
			NewHitBox->SetActorHiddenInGame(false);
			
			NewHitBox->SetActorLocation(GetOwner()->GetActorLocation());
			NewHitBox->SetLifeSpan(HitBoxLifeTime);
		}
	}
}

void UFPSHitBoxesManager::AddHitBox(UShapeComponent* NewHitBox)
{
	if (!NewHitBox || NewHitBox->GetOwner() != GetOwner())
		return;

	if (GetOwner()->Role == ENetRole::ROLE_Authority)
	{
		Hitboxes.Add(NewHitBox);
	}
}

