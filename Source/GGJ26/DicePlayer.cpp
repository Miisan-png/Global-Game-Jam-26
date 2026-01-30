#include "DicePlayer.h"
#include "DiceCamera.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"

ADicePlayer::ADicePlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(Root);
	PlaceholderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		PlaceholderMesh->SetStaticMesh(CubeMesh.Object);
		PlaceholderMesh->SetWorldScale3D(FVector(0.5f));
	}

	bShowPlaceholderInGame = false;
	NumDiceToThrow = 5;
	ThrowForce = 500.0f;
	DiceClass = ADice::StaticClass();
}

void ADicePlayer::BeginPlay()
{
	Super::BeginPlay();

	if (!bShowPlaceholderInGame)
	{
		PlaceholderMesh->SetVisibility(false);
	}
}

void ADicePlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADicePlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("ThrowDice", IE_Pressed, this, &ADicePlayer::OnThrowPressed);
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &ADicePlayer::OnThrowPressed);
}

void ADicePlayer::OnThrowPressed()
{
	ThrowDice();
}

ADiceCamera* ADicePlayer::FindCamera()
{
	TArray<AActor*> Cameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADiceCamera::StaticClass(), Cameras);

	if (Cameras.Num() > 0)
	{
		return Cast<ADiceCamera>(Cameras[0]);
	}

	return nullptr;
}

void ADicePlayer::ThrowDice()
{
	ClearDice();

	ADiceCamera* Cam = FindCamera();
	if (!Cam)
		return;

	FVector CamLocation = Cam->Camera->GetComponentLocation();
	FVector CamForward = Cam->Camera->GetForwardVector();
	FVector CamRight = Cam->Camera->GetRightVector();

	FVector SpawnBase = CamLocation + CamForward * 50.0f - FVector(0, 0, 20.0f);

	for (int32 i = 0; i < NumDiceToThrow; i++)
	{
		FVector Offset = CamRight * (i - NumDiceToThrow / 2.0f) * 15.0f;
		Offset += FVector(
			FMath::RandRange(-5.0f, 5.0f),
			FMath::RandRange(-5.0f, 5.0f),
			FMath::RandRange(-5.0f, 5.0f)
		);

		FVector SpawnLocation = SpawnBase + Offset;
		FRotator SpawnRotation = FRotator(
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f)
		);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ADice* NewDice = GetWorld()->SpawnActor<ADice>(DiceClass, SpawnLocation, SpawnRotation, Params);

		if (NewDice)
		{
			FVector ThrowDirection = CamForward + FVector(0, 0, -0.3f);
			ThrowDirection += FVector(
				FMath::RandRange(-0.1f, 0.1f),
				FMath::RandRange(-0.1f, 0.1f),
				0
			);

			NewDice->Throw(ThrowDirection, ThrowForce);
			SpawnedDice.Add(NewDice);
		}
	}
}

void ADicePlayer::ClearDice()
{
	for (ADice* D : SpawnedDice)
	{
		if (D && IsValid(D))
		{
			D->Destroy();
		}
	}
	SpawnedDice.Empty();
}

bool ADicePlayer::AreAllDiceStill()
{
	if (SpawnedDice.Num() == 0)
		return false;

	for (ADice* D : SpawnedDice)
	{
		if (D && !D->IsStill())
			return false;
	}

	return true;
}

TArray<int32> ADicePlayer::GetDiceResults()
{
	TArray<int32> Results;

	for (ADice* D : SpawnedDice)
	{
		if (D)
		{
			Results.Add(D->GetResult());
		}
	}

	return Results;
}
