#include "Dice.h"
#include "Components/StaticMeshComponent.h"

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
	FVector UpVector = GetActorUpVector();

	if (UpVector.Z > 0.7f) return 1;
	if (UpVector.Z < -0.7f) return 6;
	if (UpVector.X > 0.7f) return 2;
	if (UpVector.X < -0.7f) return 5;
	if (UpVector.Y > 0.7f) return 3;
	if (UpVector.Y < -0.7f) return 4;

	return 1;
}
