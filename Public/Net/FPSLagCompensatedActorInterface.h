// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FPSTypes.h"
#include "FPSLagCompensatedActorInterface.generated.h"

class UFPSLagCompensationComponent;

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UFPSLagCompensatedActorInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class FPSGAME_API IFPSLagCompensatedActorInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	virtual UFPSLagCompensationComponent* GetLagCompensationComp() const = 0;

	/*Call LagCompensationComponent->*/
	//virtual void SendClientHitDataToServer(uint32 ProjectileId, TArray<FProjectileHitClientData> ClientHitData) = 0;

	virtual EHitboxType GetHitboxType(UShapeComponent* HitboxComponent) const = 0;
};
