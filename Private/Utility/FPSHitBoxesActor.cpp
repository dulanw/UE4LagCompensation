// Copyright 2019 Dulan Wettasinghe. All Rights Reserved.

#include "Utility/FPSHitBoxesActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Utility/FPSActorPool.h"
#include "Player/FPSCharacterBase.h"

// Sets default values
AFPSHitBoxesActor::AFPSHitBoxesActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	HeadHitbox = CreateDefaultSubobject<UCapsuleComponent>(TEXT("HeadHitbox"));
	HeadHitbox->AttachTo(RootComponent);

	TorsoHitbox = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TorsoHitbox"));
	HeadHitbox->AttachTo(RootComponent);

	HeadHitbox->SetHiddenInGame(false);
	TorsoHitbox->SetHiddenInGame(false);
}

void AFPSHitBoxesActor::LifeSpanExpired()
{
	if (ParentPool)
		ParentPool->AddActorToPool(this);
}

void AFPSHitBoxesActor::UpdateHitBoxes(AFPSCharacterBase* InCharacter, float InTime)
{
	Time = InTime;
}

// Called when the game starts or when spawned
void AFPSHitBoxesActor::BeginPlay()
{
	Super::BeginPlay();
	Time = GetWorld()->UnpausedTimeSeconds;
}
