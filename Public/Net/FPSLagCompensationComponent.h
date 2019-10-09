// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldCollision.h"
#include "FPSTypes.h"
#include "FPSLagCompensationComponent.generated.h"

class UShapeComponent;

USTRUCT()
struct FHitboxComponentData
{
	GENERATED_BODY()

	UShapeComponent* Component;

	FTransform RelativeTransform;

	EHitboxType HitboxType;

	FHitboxComponentData() : RelativeTransform(FTransform::Identity), HitboxType(EHitboxType::None) {};

	FHitboxComponentData(UShapeComponent* InComponent, FTransform InRelTrans, EHitboxType InHitboxType)
		: Component(InComponent), RelativeTransform(InRelTrans), HitboxType(InHitboxType) {};
};

USTRUCT()
struct FSavedHitbox
{

	GENERATED_BODY()

	// Position of hitbox at time Time.
	//UPROPERTY()
	//FVector Position;

	// Rotation of hitbox at time Time.
	//UPROPERTY()
	//FQuat Rotation;

	//UPROPERTY()
	FTransform Transform;

	FCollisionShape ScaledHitBoxShape;

	// Index in the HitboxComponents;
	int32 HitboxIndex;

	//FSavedHitbox() : Position(FVector(0.0f)), Rotation(FQuat::Identity), ScaledHitBoxShape(FCollisionShape()), HitboxIndex(0) {};
	FSavedHitbox() : Transform(FTransform::Identity), ScaledHitBoxShape(FCollisionShape()), HitboxIndex(0) {};

	//FSavedHitbox(FVector InPos,	FRotator InRot,	FCollisionShape InHitboxShape, int32 InHitboxIndex)
	//	: Position(InPos), Rotation(InRot), ScaledHitBoxShape(InHitboxShape), HitboxIndex(InHitboxIndex) {};
	FSavedHitbox(FTransform  InTransform, FCollisionShape InHitboxShape, int32 InHitboxIndex)
		: Transform(InTransform), ScaledHitBoxShape(InHitboxShape), HitboxIndex(InHitboxIndex) {};

	FVector GetHitboxExtent() const
	{
		return ScaledHitBoxShape.GetExtent() * 1.25f;
	}
};


USTRUCT()
struct FSavedPosition
{

	GENERATED_BODY()

	// Position of player at time Time.
	UPROPERTY()
	FVector Position;

	// Rotation of player at time Time.
	UPROPERTY()
	FQuat Rotation;

	// Server world time when this position was updated
	UPROPERTY()
	float Time;

	// Array of Saved Hitbox data for this position
	UPROPERTY()
	TArray<FSavedHitbox> Hitboxes;

	FSavedPosition() :
		Position(FVector(0.0f)),
		Rotation(FQuat::Identity),
		Time(0.f),
		Hitboxes() {};

	FSavedPosition(
		FVector InPos,
		FQuat InRot,
		float InTime,
		TArray<FSavedHitbox> InHitboxes
	) :
		Position(InPos),
		Rotation(InRot),
		Time(InTime),
		Hitboxes(InHitboxes) {};

};

//UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
UCLASS()
class FPSGAME_API UFPSLagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFPSLagCompensationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	/*NEVER ADD COLLISION WITH NEGATIVE SCALE*/
	UFUNCTION(BlueprintCallable)
	void AddHitbox(UShapeComponent* NewHitBox, const EHitboxType HitboxType = EHitboxType::None);

	/*Recieve the client-side hit data from the client*/
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ReceiveClientHitData(uint32 ProjectileId, const TArray<FProjectileClientHitData>& ClientHitData);

	void RewindHitboxes(float RewindTime);
	void RestoreHitboxes();

	/*Get the ProjectileHitData, this should be called by the projectile owner on hit clients, pass the the client hitdata*/
	bool IsValidClientHit(FProjectileClientHitData& InClientHit, FProjectileHitData& OutProjectileValidHitData, float RewindTime);

protected:
	///@brief Called from within PlayerCharacterMovement.
	/// Adds an instance of an FSavedPosition to the SavedPositions array.
	void PositionUpdated();

	//TMap<TWeakObjectPtr<const UShapeComponent>, const FHitboxComponentData> HitboxComponents;
	TArray<FHitboxComponentData> HitboxComponents;

	FSavedPosition BuildSavedPosition();

	/*Build an array of FSavedHitbox for each saved Hitbox*/
	TArray<FSavedHitbox> BuildSavedHitboxArr();

	TArray<FSavedPosition> SavedPositions;

	///@brief Maximum time to hold onto SavedPositions.
	///       300ms of Lag Compensation.
	//UPROPERTY(EditDefaultsOnly)
	//float MaxLagCompensationTime;

	void DrawSavedPositions(const TArray<FSavedPosition> SavedPositions, const float DeltaTime);
	void DrawSavedHitboxes(const TArray<FSavedHitbox> SavedHitboxes, const FColor BoxColor = FColor::Red, const bool PersistentLines = false, const float BoxLifetime = -1.0f, const uint8 DepthPriority = 0, const float BoxThickness = 0.0f) const;

	void RewindHitbox(const FSavedPosition& RewindPosition);
	void RewindHitboxLerp(const FSavedPosition& RewindPosition1, const FSavedPosition& RewindPosition2, float Alpha);

	void CleanUpHitboxHistory();

	TArray<FSavedHitbox> GetRewindHitboxes(float RewindTime);
	TArray<FSavedHitbox> GetRewindHitboxesLerp(const FSavedPosition& RewindPosition1, const FSavedPosition& RewindPosition2, float Alpha);

	bool IsPointInShape(const ECollisionShape::Type ShapeType, const FVector& Extent, const FTransform& WorldTransform, const FVector& Point, const float Inflation = 1.0f) const;

	bool IsPointInBox(const FTransform WorldTransform, const FVector& Extent, const FVector Point) const;
	bool IsPointInCapsule(const FTransform WorldTransform, const FVector& Extent, const FVector Point) const;

	float CalculateRewindTime(float InPing) const;

protected:
	TArray<FProjectileData> ClientProjectiles;

	/*The last USED projectile id, +1 to get unused one*/
	uint32 LatestProjectileId;

public:
	/*Increament and return new id, should only be on the local client and sent to the server*/
	uint32 GetUnusedProjectileId() { return (++LatestProjectileId); }

	void AddClientProjectileData(FProjectileData ServerProjectile);
};
