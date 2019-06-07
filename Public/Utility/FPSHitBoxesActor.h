// Copyright 2019 Dulan Wettasinghe. All Rights Reserved.

#pragma once


#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utility/FPSPooledActorInterface.h"
#include "FPSHitBoxesActor.generated.h"

class UCapsuleComponent;
class AFPSCharacterBase;

UCLASS()
class FPSGAME_API AFPSHitBoxesActor : public AActor, public IFPSPooledActorInterface 
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFPSHitBoxesActor();

	/** Called when the lifespan of an actor expires (if he has one). */
	virtual void LifeSpanExpired() override;
	virtual void UpdateHitBoxes(AFPSCharacterBase* InCharacter, float InTime);
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* HeadHitbox;

	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* TorsoHitbox;
	
private:
	float Time;

};
