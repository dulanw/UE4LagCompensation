// Copyright 2019 Dulan Wettasinghe. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FPSHitBoxesManager.generated.h"

class UFPSActorPool;
class ACharacter;
class UShapeComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FPSGAME_API UFPSHitBoxesManager : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFPSHitBoxesManager(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void AddHitBox(UShapeComponent* NewHitBox);


protected:
	UPROPERTY()
	UFPSActorPool* HitBoxPool;

	/*Maximum lag compensation time in millieseconds, default = 150*/
	float CompensationTime;

	float HitBoxLifeTime;

	UPROPERTY(EditAnywhere, Category = "Hitbox")
	TArray<UShapeComponent*> Hitboxes;
};
