#include "DiceCamera.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameModeDice.h"

ADiceCamera::ADiceCamera()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);

	BreathingSpeed = 2.0f;
	BreathingAmplitudeZ = 1.5f;
	BreathingAmplitudeRotation = 0.3f;
	bEnableBreathing = true;
	bIsMainCamera = true;

	TimeAccumulator = 0.0f;
}

void ADiceCamera::BeginPlay()
{
	Super::BeginPlay();

	InitialPosition = Camera->GetRelativeLocation();
	InitialRotation = Camera->GetRelativeRotation();

	if (bIsMainCamera)
	{
		AGameModeDice* GM = Cast<AGameModeDice>(UGameplayStatics::GetGameMode(this));
		if (GM)
		{
			GM->SetMainCamera(this);
		}
	}
}

void ADiceCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bEnableBreathing)
		return;

	TimeAccumulator += DeltaTime;

	float BreathValue = FMath::Sin(TimeAccumulator * BreathingSpeed);
	float BreathValueSecondary = FMath::Sin(TimeAccumulator * BreathingSpeed * 0.7f);

	FVector NewPosition = InitialPosition;
	NewPosition.Z += BreathValue * BreathingAmplitudeZ;

	FRotator NewRotation = InitialRotation;
	NewRotation.Pitch += BreathValue * BreathingAmplitudeRotation;
	NewRotation.Roll += BreathValueSecondary * BreathingAmplitudeRotation * 0.5f;

	Camera->SetRelativeLocation(NewPosition);
	Camera->SetRelativeRotation(NewRotation);
}

void ADiceCamera::ActivateCamera()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->SetViewTargetWithBlend(this, 0.5f);
	}
}
