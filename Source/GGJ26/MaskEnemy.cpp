#include "MaskEnemy.h"
#include "Components/StaticMeshComponent.h"

AMaskEnemy::AMaskEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CubeMesh.Object);
	}

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BobSpeed = 1.5f;
	BobAmplitude = 5.0f;
	MeshScale = FVector(0.5f);
	TimeAccumulator = 0.0f;
}

void AMaskEnemy::BeginPlay()
{
	Super::BeginPlay();

	Mesh->SetWorldScale3D(MeshScale);
	InitialLocation = GetActorLocation();
}

void AMaskEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeAccumulator += DeltaTime;

	float BobOffset = FMath::Sin(TimeAccumulator * BobSpeed) * BobAmplitude;
	FVector NewLocation = InitialLocation;
	NewLocation.Z += BobOffset;

	SetActorLocation(NewLocation);
}
