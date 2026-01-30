#include "Dice.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

ADice::ADice()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	DiceSize = 0.15f;
	bShowDebugNumbers = true;

	Mesh->SetWorldScale3D(FVector(DiceSize));
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(true);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetNotifyRigidBodyCollision(true);

	Mesh->SetLinearDamping(0.5f);
	Mesh->SetAngularDamping(0.5f);

	bHasBeenThrown = false;
}

void ADice::BeginPlay()
{
	Super::BeginPlay();
}

void ADice::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowDebugNumbers)
	{
		DrawFaceNumbers();
	}
}

void ADice::Throw(FVector Direction, float Force)
{
	bHasBeenThrown = true;

	Direction.Normalize();
	Mesh->AddImpulse(Direction * Force, NAME_None, true);

	FVector RandomTorque = FVector(
		FMath::RandRange(-1.0f, 1.0f),
		FMath::RandRange(-1.0f, 1.0f),
		FMath::RandRange(-1.0f, 1.0f)
	) * Force * 0.5f;

	Mesh->AddAngularImpulseInDegrees(RandomTorque, NAME_None, true);
}

bool ADice::IsStill()
{
	if (!bHasBeenThrown)
		return false;

	FVector Velocity = Mesh->GetPhysicsLinearVelocity();
	FVector AngularVelocity = Mesh->GetPhysicsAngularVelocityInDegrees();

	return Velocity.Size() < 1.0f && AngularVelocity.Size() < 1.0f;
}

int32 ADice::GetResult()
{
	float HighestDot = -2.0f;
	int32 TopFace = 1;

	for (int32 i = 1; i <= 6; i++)
	{
		FVector WorldNormal = GetActorRotation().RotateVector(GetFaceNormal(i));
		float Dot = FVector::DotProduct(WorldNormal, FVector::UpVector);

		if (Dot > HighestDot)
		{
			HighestDot = Dot;
			TopFace = i;
		}
	}

	return TopFace;
}

void ADice::DrawFaceNumbers()
{
	float CubeExtent = 50.0f * DiceSize;
	int32 TopFace = GetResult();

	for (int32 i = 1; i <= 6; i++)
	{
		FVector LocalCenter = GetFaceNormal(i) * CubeExtent;
		FVector WorldCenter = GetActorLocation() + GetActorRotation().RotateVector(LocalCenter);

		FColor Color = (i == TopFace) ? FColor::Green : FColor::White;
		FString Num = FString::FromInt(i);

		DrawDebugString(GetWorld(), WorldCenter, Num, nullptr, Color, 0.0f, true, 2.0f);

		FVector LineEnd = WorldCenter + GetActorRotation().RotateVector(GetFaceNormal(i)) * 10.0f;
		DrawDebugLine(GetWorld(), WorldCenter, LineEnd, Color, false, 0.0f, 0, 1.0f);
	}

	FVector TopIndicator = GetActorLocation() + FVector(0, 0, CubeExtent + 20.0f);
	DrawDebugString(GetWorld(), TopIndicator, FString::Printf(TEXT("[%d]"), TopFace), nullptr, FColor::Yellow, 0.0f, true, 3.0f);
}

FVector ADice::GetFaceNormal(int32 FaceIndex)
{
	switch (FaceIndex)
	{
		case 1: return FVector(0, 0, 1);
		case 2: return FVector(1, 0, 0);
		case 3: return FVector(0, 1, 0);
		case 4: return FVector(0, -1, 0);
		case 5: return FVector(-1, 0, 0);
		case 6: return FVector(0, 0, -1);
		default: return FVector(0, 0, 1);
	}
}

FVector ADice::GetFaceCenter(int32 FaceIndex)
{
	return GetFaceNormal(FaceIndex) * 50.0f * DiceSize;
}

int32 ADice::GetFaceValueFromDirection(FVector LocalDirection)
{
	LocalDirection.Normalize();

	float BestDot = -2.0f;
	int32 BestFace = 1;

	for (int32 i = 1; i <= 6; i++)
	{
		float Dot = FVector::DotProduct(LocalDirection, GetFaceNormal(i));
		if (Dot > BestDot)
		{
			BestDot = Dot;
			BestFace = i;
		}
	}

	return BestFace;
}
