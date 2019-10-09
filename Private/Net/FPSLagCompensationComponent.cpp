// Fill out your copyright notice in the Description page of Project Settings.


#include "Net/FPSLagCompensationComponent.h"
#include "Net/FPSLagCompensatedActorInterface.h"
#include "Components/ShapeComponent.h"
#include "DrawDebugHelpers.h"
#include "Player/FPSPlayerState.h"
#include "FPSGameInstance.h"


// CVars
namespace LagCompensationCVars
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static int32 NetLagCompensationDebugDrawing = 1;
	FAutoConsoleVariableRef CVarNetLagCompensationDebugDrawing(
		TEXT("p.NetLagCompensationDebugDrawing"),
		NetLagCompensationDebugDrawing,
		TEXT("Useful for testing lag compensation rewind code.\n")
		TEXT("<=0: Disable, 1: Draw Debug boxes"),
		ECVF_Cheat);

#endif /*!(UE_BUILD_SHIPPING || UE_BUILD_TEST)*/
}


UFPSLagCompensationComponent::UFPSLagCompensationComponent(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
	: Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	SetIsReplicated(true);

	// ...
	//UCapsuleComponent

	LatestProjectileId = 0;
}

// Called when the game starts
void UFPSLagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UFPSLagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner()->HasAuthority())
	{
		PositionUpdated();
		DrawSavedPositions(SavedPositions, DeltaTime);
		CleanUpHitboxHistory();
	}
}

void UFPSLagCompensationComponent::PositionUpdated()
{
	SavedPositions.Add(BuildSavedPosition());
}

FSavedPosition UFPSLagCompensationComponent::BuildSavedPosition()
{
	const FVector LocationToSave = GetOwner()->GetActorLocation();
	const FQuat RotationToSave = GetOwner()->GetActorQuat();
	const float WorldTime = GetWorld()->GetTimeSeconds();

	const TArray<FSavedHitbox> HitboxesToSave = BuildSavedHitboxArr();

	return FSavedPosition(LocationToSave, RotationToSave, WorldTime, HitboxesToSave);
}

TArray<FSavedHitbox> UFPSLagCompensationComponent::BuildSavedHitboxArr()
{
	TArray<FSavedHitbox> SavedHitboxArr;

	//for (TTuple<TWeakObjectPtr<const UShapeComponent>, const EHitboxType>& HitboxElement : HitboxComponents)
	//for (int32 i = 0; i < HitboxComponents.Num(); i++)
	//int32 index = 0;
	//for (TTuple<TWeakObjectPtr<const UShapeComponent>, const FHitboxComponentData>& HitboxElement : HitboxComponents)

	for (int32 index = 0; index < HitboxComponents.Num(); index++)
	{
		UShapeComponent* HitboxComponent = HitboxComponents[index].Component;
		FSavedHitbox SHB_NewHitbox;
		SHB_NewHitbox.HitboxIndex = index;
		SHB_NewHitbox.ScaledHitBoxShape = HitboxComponent->GetCollisionShape();
		SHB_NewHitbox.Transform = HitboxComponent->GetComponentTransform();

		SavedHitboxArr.Add(SHB_NewHitbox);
	}

	return SavedHitboxArr;
}

void UFPSLagCompensationComponent::AddHitbox(UShapeComponent* NewHitBox, const EHitboxType HitboxType /*= EHitboxType::None*/)
{
	if (NewHitBox)
	{
		FHitboxComponentData HitboxData(NewHitBox, NewHitBox->GetRelativeTransform(), HitboxType);
		HitboxComponents.Add(HitboxData);
	}
}

void UFPSLagCompensationComponent::Server_ReceiveClientHitData_Implementation(uint32 ProjectileId, const TArray<FProjectileClientHitData>& ClientHitData)
{
	int32 ProjectileIndex = 0;
	APlayerState* PlayerState = Cast<APawn>(GetOwner())->GetPlayerState();

	if (!ClientProjectiles.Find(FProjectileData(ProjectileId), ProjectileIndex) || !PlayerState)
	{
		return;
	}

	FProjectileData& ProjectileData = ClientProjectiles[ProjectileIndex];

	if (ProjectileData.bWantServerRewind || !ProjectileData.Projectile.IsValid())
		return;

	for (FProjectileClientHitData Data : ClientHitData)
	{
		if (!IsValid(Data.HitActor))
		{
			continue;
		}

		IFPSLagCompensatedActorInterface* LagCompActor = Cast<IFPSLagCompensatedActorInterface>(Data.HitActor);
		UFPSLagCompensationComponent* LagComp = (LagCompActor) ? LagCompActor->GetLagCompensationComp() : nullptr;

		if (!LagComp)
			continue;

		FProjectileHitData HitData(Data.HitActor);
		LagComp->IsValidClientHit(Data, HitData, CalculateRewindTime(PlayerState->ExactPing));
	}
}

bool UFPSLagCompensationComponent::Server_ReceiveClientHitData_Validate(uint32 ProjectileId, const TArray<FProjectileClientHitData>& ClientHitData)
{
	return true;
}

void UFPSLagCompensationComponent::RewindHitboxes(float RewindTime)
{
	if (!GetOwner()->HasAuthority() || SavedPositions.Num() < 1)
		return;

	float RewindedTime = GetWorld()->GetTimeSeconds() - RewindTime;

	/*if the rewind time is older than the oldest saved pos then set it to the oldest possible, then the user will need to compensate by aiming in front*/
	if (RewindedTime <= SavedPositions[0].Time)
	{
		RewindHitbox(SavedPositions[0]);
	}
	else
	{
		for (int32 i = 0; i < SavedPositions.Num(); i++)
		{
			if (RewindTime == SavedPositions[i].Time)
			{
				RewindHitbox(SavedPositions[i]);
				break;
			}
			else if (RewindedTime > SavedPositions[i].Time)
			{
				if (i < SavedPositions.Num() - 1 && RewindedTime < SavedPositions[i + 1].Time)
				{
					const FSavedPosition& RewindPosition1 = SavedPositions[i];
					const FSavedPosition& RewindPosition2 = SavedPositions[i + 1];

					const float Alpha = FMath::GetRangePct(RewindPosition1.Time, RewindPosition2.Time, RewindedTime);
					RewindHitboxLerp(RewindPosition1, RewindPosition2, Alpha);
					break;
				}
				else
				{
					//#TODO if this is the most recent one, which might be older by 1 frame.
				}
			}
		}
	}
}

void UFPSLagCompensationComponent::RestoreHitboxes()
{
	if (!GetOwner()->HasAuthority())
		return;

	for (FHitboxComponentData hitbox : HitboxComponents)
	{
		hitbox.Component->SetRelativeTransform(hitbox.RelativeTransform);
	}
}

bool UFPSLagCompensationComponent::IsValidClientHit(FProjectileClientHitData& InClientHit, FProjectileHitData& OutProjectileValidHitData, float RewindTime)
{
	if (InClientHit.HitActor != GetOwner())
		return false;

	TArray<FSavedHitbox> Hitboxes = GetRewindHitboxes(RewindTime);

	bool bHasValidHit = false;

	for (FSavedHitbox Hitbox : Hitboxes)
	{
		for (FVector HitLocation : InClientHit.HitLocations)
		{
			DrawDebugSphere(GetWorld(), HitLocation, 1.0f, 12, FColor::Blue, true, 100.0f, 0);

			if (IsPointInShape(Hitbox.ScaledHitBoxShape.ShapeType, Hitbox.GetHitboxExtent(), Hitbox.Transform, HitLocation))
			{
				//IsPointInShape(Hitbox.ScaledHitBoxShape, Hitbox.Transform, HitLocation, 1.1f)
				bHasValidHit = true;
				UE_LOG(LogTemp, Warning, TEXT("ConfirmedHit"));
			}

		}
	}


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (LagCompensationCVars::NetLagCompensationDebugDrawing)
	{
		if (RewindTime > 0.0f)
			DrawSavedHitboxes(BuildSavedHitboxArr(), FColor::Blue, true);

		if (bHasValidHit)
		{
			DrawSavedHitboxes(Hitboxes, FColor::Red, true);
		}
		else
		{
			DrawSavedHitboxes(Hitboxes, FColor::Green, true);
		}
	}
#endif /*!(UE_BUILD_SHIPPING || UE_BUILD_TEST)*/

	return false;
}

void UFPSLagCompensationComponent::DrawSavedPositions(const TArray<FSavedPosition> SavedPositions, const float DeltaTime) {

	const FColor BoxColor = FColor::Blue;
	const float BoxLifetime = DeltaTime;
	const uint8 DepthPriority = 0;
	const float BoxThickness = 0.75f;
	const bool PersistentLines = false;

	for (FSavedPosition SavedPosition : SavedPositions) 
	{
		//UE_LOG(LogTemp, Warning, TEXT("%d"), SavedPositions.Num());

		DrawSavedHitboxes(SavedPosition.Hitboxes, BoxColor, PersistentLines, BoxLifetime, DepthPriority, BoxThickness);
	}
}

void UFPSLagCompensationComponent::DrawSavedHitboxes(const TArray<FSavedHitbox> SavedHitboxes, const FColor BoxColor /*= FColor::Red*/, const bool PersistentLines /*= false*/, const float BoxLifetime /*= -1.0f*/, const uint8 DepthPriority /*= 0*/, const float BoxThickness /*= 0.0f*/) const
{
	for (FSavedHitbox SavedHitbox : SavedHitboxes)
	{
		DrawDebugBox(
			GetWorld(),
			SavedHitbox.Transform.GetTranslation(),
			SavedHitbox.GetHitboxExtent(),
			SavedHitbox.Transform.GetRotation(),
			BoxColor,
			PersistentLines,
			BoxLifetime,
			DepthPriority
		);
	}
}

void UFPSLagCompensationComponent::RewindHitbox(const FSavedPosition& RewindPosition)
{
	for (FSavedHitbox hitbox : RewindPosition.Hitboxes)
	{
		UShapeComponent* HitboxComp = HitboxComponents[hitbox.HitboxIndex].Component;
		//HitboxComp->SetWorldLocationAndRotation(hitbox.Position, hitbox.Rotation, false);
		HitboxComp->SetWorldTransform(hitbox.Transform);
	}
}

void UFPSLagCompensationComponent::RewindHitboxLerp(const FSavedPosition& RewindPosition1, const FSavedPosition& RewindPosition2, float Alpha)
{
	/*not dealing with dynamic removal of hitboxes*/
	if (RewindPosition1.Hitboxes.Num() != RewindPosition2.Hitboxes.Num())
		return;

	for (int32 i = 0; i < RewindPosition1.Hitboxes.Num(); i++)
	{
		const FSavedHitbox& HitboxPosition1 = RewindPosition1.Hitboxes[i];
		const FSavedHitbox& HitboxPosition2 = RewindPosition2.Hitboxes[i];

		//FVector Location = FMath::Lerp(RewindPosition1.Hitboxes[i].Position, RewindPosition2.Hitboxes[i].Position, Alpha);
		//FQuat Rotation = FMath::Lerp(RewindPosition1.Hitboxes[i].Rotation, RewindPosition2.Hitboxes[i].Rotation, Alpha);
		//FTransform Transform = FMath::Lerp(HitboxPosition1.Transform, HitboxPosition2.Transform, Alpha);
		FQuat LerpRotation = FMath::Lerp(HitboxPosition1.Transform.GetRotation(), HitboxPosition2.Transform.GetRotation(), Alpha);
		FVector LerpLocation = FMath::Lerp(HitboxPosition1.Transform.GetTranslation(), HitboxPosition2.Transform.GetTranslation(), Alpha);
		FVector LerpScale3D = FMath::Lerp(HitboxPosition1.Transform.GetScale3D(), HitboxPosition2.Transform.GetScale3D(), Alpha);
		FTransform LerpTransform(LerpRotation, LerpLocation, LerpScale3D);

		UShapeComponent* HitboxComp = HitboxComponents[HitboxPosition1.HitboxIndex].Component;
		//HitboxComp->SetWorldLocationAndRotation(Location, Rotation, false);
		HitboxComp->SetWorldTransform(LerpTransform);
	}
}

void UFPSLagCompensationComponent::CleanUpHitboxHistory()
{
	const float WorldTime = GetWorld()->GetTimeSeconds();
	UFPSGameInstance* GInstance = GetWorld()->GetGameInstance<UFPSGameInstance>();

	// Clean up SavedPositions that have exceeded out Age limit.
	// However, we should keep a handle to at least one FSavedPosition that exceeds the max age
	// for interpolation.
	if (SavedPositions.Num() >= 2 /*&& SavedPositions[1].Time < WorldTime - MaxLagCompensationTime*/)
	{
		//clear the previous out of date ones, do not remove the last one no matter what.
		for (int32 i = 0; i < SavedPositions.Num() - 1; i++)
		{
			if (SavedPositions[i].Time < WorldTime - GInstance->NetMaxLagCompensationTime)
				SavedPositions.RemoveAt(i);
		}
	}
}

TArray<FSavedHitbox> UFPSLagCompensationComponent::GetRewindHitboxes(float RewindTime)
{
	TArray<FSavedHitbox> Hitboxes;

	if (!GetOwner()->HasAuthority() || SavedPositions.Num() < 1)
		return Hitboxes;

	const float WorldTimeSeconds = GetWorld()->GetTimeSeconds();
	const float RewindedTime = WorldTimeSeconds - RewindTime;

	/*if the rewind time is older than the oldest saved pos then set it to the oldest possible, then the user will need to compensate by aiming in front*/
	if (RewindedTime <= SavedPositions[0].Time)
	{
		return SavedPositions[0].Hitboxes;
	}
	/*If we haven't ticked yet and haven't got the latest saved positions*/
	else if (WorldTimeSeconds > SavedPositions.Last().Time)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Latest Called"));
		if (RewindedTime >= WorldTimeSeconds)
			return BuildSavedHitboxArr();

		const FSavedPosition& RewindPosition1 = SavedPositions.Last();
		const FSavedPosition& RewindPosition2 = BuildSavedPosition();
		const float Alpha = FMath::GetRangePct(RewindPosition1.Time, RewindPosition2.Time, RewindedTime);

		return GetRewindHitboxesLerp(RewindPosition1, RewindPosition2, Alpha);
	}
	else
	{
		for (int32 i = 0; i < SavedPositions.Num(); i++)
		{
			if (RewindedTime == SavedPositions[i].Time)
			{
				return SavedPositions[i].Hitboxes;
			}
			else if (RewindedTime < SavedPositions[i].Time)
			{
				if (i - 1 > 0 && RewindedTime > SavedPositions[i - 1].Time)
				{
					const FSavedPosition& RewindPosition1 = SavedPositions[i - 1];
					const FSavedPosition& RewindPosition2 = SavedPositions[i];

					const float Alpha = FMath::GetRangePct(RewindPosition1.Time, RewindPosition2.Time, RewindedTime);
					return GetRewindHitboxesLerp(RewindPosition1, RewindPosition2, Alpha);
				}
				else
				{
					//#TODO if this is the most recent one, which might be older by 1 frame.
					//should be taken care of by else if (RewindedTime >= SavedPositions.Last().Time)
				}
			}
		}
	}

	return Hitboxes;
}

TArray<FSavedHitbox> UFPSLagCompensationComponent::GetRewindHitboxesLerp(const FSavedPosition& RewindPosition1, const FSavedPosition& RewindPosition2, float Alpha)
{
	TArray<FSavedHitbox> Hitboxes;

	/*not dealing with dynamic removal of hitboxes*/
	if (RewindPosition1.Hitboxes.Num() != RewindPosition2.Hitboxes.Num())
		return Hitboxes;

	for (int32 i = 0; i < RewindPosition1.Hitboxes.Num(); i++)
	{
		const FSavedHitbox& HitboxPosition1 = RewindPosition1.Hitboxes[i];
		const FSavedHitbox& HitboxPosition2 = RewindPosition2.Hitboxes[i];
		FSavedHitbox LerpHitbox = RewindPosition1.Hitboxes[i];


		//Hitbox.Position = FMath::Lerp(RewindPosition1.Hitboxes[i].Position, RewindPosition2.Hitboxes[i].Position, Alpha);
		//Hitbox.Rotation = FMath::Lerp(RewindPosition1.Hitboxes[i].Rotation, RewindPosition2.Hitboxes[i].Rotation, Alpha);
		//Hitbox.Transform = FMath::Lerp(HitboxPosition1.Transform, HitboxPosition2.Transform, Alpha);

		//UKismetMathLibrary

		FQuat LerpRotation = FMath::Lerp(HitboxPosition1.Transform.GetRotation(), HitboxPosition2.Transform.GetRotation(), Alpha);
		FVector LerpLocation = FMath::Lerp(HitboxPosition1.Transform.GetTranslation(), HitboxPosition2.Transform.GetTranslation(), Alpha);
		FVector LerpScale3D = FMath::Lerp(HitboxPosition1.Transform.GetScale3D(), HitboxPosition2.Transform.GetScale3D(), Alpha);
		LerpHitbox.Transform = FTransform(LerpRotation, LerpLocation, LerpScale3D);

		Hitboxes.Add(LerpHitbox);
	}

	return Hitboxes;
}

bool UFPSLagCompensationComponent::IsPointInShape(const ECollisionShape::Type ShapeType, const FVector& Extent, const FTransform& WorldTransform, const FVector& Point, const float Inflation /*= 1.0f*/) const
{
	if (ShapeType == ECollisionShape::Box)
	{
		return IsPointInBox(WorldTransform, (Extent * Inflation), Point);
	}
	else if (ShapeType == ECollisionShape::Capsule)
	{
		return IsPointInBox(WorldTransform, (Extent * Inflation), Point);
		//return IsPointInCapsule(Shape, WorldTransform, Point);
	}

	return false;
}

bool UFPSLagCompensationComponent::IsPointInBox(const FTransform WorldTransform, const FVector& Extent, const FVector Point) const
{
	//UKismetMathLibrary::IsPointInBoxWithTransform
	// Put point in component space
	const FVector PointInComponentSpace = WorldTransform.InverseTransformPosition(Point);
	// Now it's just a normal point-in-box test, with a box at the origin.
	const FBox Box(-Extent, Extent);
	return Box.IsInsideOrOn(PointInComponentSpace);
}

bool UFPSLagCompensationComponent::IsPointInCapsule(const FTransform WorldTransform, const FVector& Extent, const FVector Point) const
{
	return false;
}

float UFPSLagCompensationComponent::CalculateRewindTime(float InPing) const
{
	//UE_LOG(LogTemp, Warning, TEXT("World Delta Seconds: %f"), GetWorld()->GetDeltaSeconds());
	UFPSGameInstance* GInstance = GetWorld()->GetGameInstance<UFPSGameInstance>();

	float PingSeconds = (InPing / 1000.0f) - GetWorld()->GetDeltaSeconds();
	const float ClampedPingSeconds = FMath::Clamp(PingSeconds, 0.0f, GInstance->NetMaxLagCompensationTime);

	return (ClampedPingSeconds > 0.0f) ? ClampedPingSeconds : 0.0f;
}

void UFPSLagCompensationComponent::AddClientProjectileData(FProjectileData ServerProjectile)
{
	ClientProjectiles.Add(ServerProjectile);

	if (ServerProjectile.ProjectileID > LatestProjectileId)
	{
		LatestProjectileId = ServerProjectile.ProjectileID;
	}
}
