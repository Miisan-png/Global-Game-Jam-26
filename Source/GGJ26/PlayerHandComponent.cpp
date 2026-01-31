#include "PlayerHandComponent.h"
#include "DiceCamera.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

UPlayerHandComponent::UPlayerHandComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Default mesh names - change these to match your blueprint
	PalmMeshName = TEXT("palm");
	Finger1Name = TEXT("f1");  // Thumb
	Finger2Name = TEXT("f2");  // Index
	Finger3Name = TEXT("f3");  // Middle
	Finger4Name = TEXT("f4");  // Ring
	Finger5Name = TEXT("f5");  // Pinky
	Knife1Name = TEXT("f1_knife");
	Knife2Name = TEXT("f2_knife");
	Knife3Name = TEXT("f3_knife");
	Knife4Name = TEXT("f4_knife");
	Knife5Name = TEXT("f5_knife");

	// Default animation values
	KnifeStartOffset = FVector(0, 0, 150.0f);  // Start way up, out of view

	// Per-finger positions
	Knife5DownPos = FVector(7.327833f, 3.111940f, 10.908005f);  // Pinky
	Knife5SlicePos = FVector(10.020372f, 6.436943f, 9.133961f);

	Knife4DownPos = FVector(5.0f, 2.5f, 12.0f);  // Ring - adjust as needed
	Knife4SlicePos = FVector(7.5f, 4.0f, 11.0f);

	Knife3DownPos = FVector(2.728193f, 1.191969f, 15.356427f);  // Middle
	Knife3SlicePos = FVector(4.848198f, 1.266445f, 15.152472f);

	Knife2DownPos = FVector(-1.463493f, 1.447959f, 15.356079f);  // Index
	Knife2SlicePos = FVector(0.380462f, 1.578284f, 15.495083f);

	Knife1DownPos = FVector(-3.659707f, 1.035197f, 13.988920f);  // Thumb
	Knife1SlicePos = FVector(-1.593682f, -0.494915f, 14.483620f);

	KnifeDownSpeed = 3.0f;   // Slower - takes ~0.3s
	KnifeSliceSpeed = 5.0f;  // Slower slice - takes ~0.2s
	WaitTimeBeforeSlice = 0.3f;  // Longer pause for tension
	FingerImpulseStrength = 300.0f;
	KnifeReturnSpeed = 2.0f;  // Slow return
	CamShakeIntensity = 3.0f;  // Subtle shake on enemy cut
	CamShakeDuration = 0.25f;  // Quick shake
	KnifeSliceRotation = 15.0f;  // Degrees to rotate during slice

	bCameraShaking = false;
	CameraShakeTimer = 0.0f;

	// Hand settings
	bIsPlayerHand = true;

	// Blood VFX defaults
	BloodParticleSystem = nullptr;
	BloodColor = FLinearColor(0.5f, 0.0f, 0.0f, 1.0f);  // Dark red
	BloodSplatterCount = 5;
	BloodSplatterSpread = 20.0f;

	// Camera zoom (subtle for enemy hand)
	ChopCameraZoomAmount = 25.0f;  // Subtle zoom
	ChopCameraZoomSpeed = 3.0f;  // Smooth speed
	bCameraZooming = false;
	CameraZoomProgress = 0.0f;
	bCameraZoomingOut = false;
	CameraZoomOriginalPos = FVector::ZeroVector;
	CameraZoomOriginalRot = FRotator::ZeroRotator;
	ShakeOffset = FVector::ZeroVector;
	CachedDiceCamera = nullptr;

	CurrentFingerToChop = 5;  // Start with pinky
	FingersRemaining = 5;

	bIsAnimating = false;
	AnimationPhase = 0;
	AnimationTimer = 0.0f;
	CurrentKnife = nullptr;
	CurrentFinger = nullptr;

	PalmMesh = nullptr;
	Finger1 = nullptr;
	Finger2 = nullptr;
	Finger3 = nullptr;
	Finger4 = nullptr;
	Finger5 = nullptr;
	Knife1 = nullptr;
	Knife2 = nullptr;
	Knife3 = nullptr;
	Knife4 = nullptr;
	Knife5 = nullptr;
}

void UPlayerHandComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find meshes by name in the owner actor
	AActor* Owner = GetOwner();
	if (Owner)
	{
		TArray<UStaticMeshComponent*> MeshComponents;
		Owner->GetComponents<UStaticMeshComponent>(MeshComponents);

		for (UStaticMeshComponent* Mesh : MeshComponents)
		{
			FString Name = Mesh->GetName();

			if (Name.Equals(PalmMeshName, ESearchCase::IgnoreCase)) PalmMesh = Mesh;
			else if (Name.Equals(Finger1Name, ESearchCase::IgnoreCase)) Finger1 = Mesh;
			else if (Name.Equals(Finger2Name, ESearchCase::IgnoreCase)) Finger2 = Mesh;
			else if (Name.Equals(Finger3Name, ESearchCase::IgnoreCase)) Finger3 = Mesh;
			else if (Name.Equals(Finger4Name, ESearchCase::IgnoreCase)) Finger4 = Mesh;
			else if (Name.Equals(Finger5Name, ESearchCase::IgnoreCase)) Finger5 = Mesh;
			else if (Name.Equals(Knife1Name, ESearchCase::IgnoreCase)) Knife1 = Mesh;
			else if (Name.Equals(Knife2Name, ESearchCase::IgnoreCase)) Knife2 = Mesh;
			else if (Name.Equals(Knife3Name, ESearchCase::IgnoreCase)) Knife3 = Mesh;
			else if (Name.Equals(Knife4Name, ESearchCase::IgnoreCase)) Knife4 = Mesh;
			else if (Name.Equals(Knife5Name, ESearchCase::IgnoreCase)) Knife5 = Mesh;
		}

		// Log what we found
		UE_LOG(LogTemp, Log, TEXT("PlayerHand - Found meshes: Palm=%s, F1=%s, F2=%s, F3=%s, F4=%s, F5=%s"),
			PalmMesh ? TEXT("Yes") : TEXT("No"),
			Finger1 ? TEXT("Yes") : TEXT("No"),
			Finger2 ? TEXT("Yes") : TEXT("No"),
			Finger3 ? TEXT("Yes") : TEXT("No"),
			Finger4 ? TEXT("Yes") : TEXT("No"),
			Finger5 ? TEXT("Yes") : TEXT("No"));

		UE_LOG(LogTemp, Log, TEXT("PlayerHand - Knives: K1=%s, K2=%s, K3=%s, K4=%s, K5=%s"),
			Knife1 ? TEXT("Yes") : TEXT("No"),
			Knife2 ? TEXT("Yes") : TEXT("No"),
			Knife3 ? TEXT("Yes") : TEXT("No"),
			Knife4 ? TEXT("Yes") : TEXT("No"),
			Knife5 ? TEXT("Yes") : TEXT("No"));
	}

	SetupInputBindings();

	// Move all knives up to start position
	TArray<UStaticMeshComponent*> Knives = { Knife1, Knife2, Knife3, Knife4, Knife5 };
	for (UStaticMeshComponent* Knife : Knives)
	{
		if (Knife)
		{
			FVector Pos = Knife->GetRelativeLocation();
			Pos += KnifeStartOffset;
			Knife->SetRelativeLocation(Pos);
		}
	}
}

void UPlayerHandComponent::SetupInputBindings()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && GetOwner())
	{
		GetOwner()->EnableInput(PC);
		UInputComponent* InputComp = GetOwner()->InputComponent;
		if (InputComp)
		{
			InputComp->BindKey(EKeys::K, IE_Pressed, this, &UPlayerHandComponent::TriggerChop);
		}
	}
}

void UPlayerHandComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsAnimating)
	{
		UpdateAnimation(DeltaTime);
	}

	if (bCameraShaking)
	{
		UpdateCameraShake(DeltaTime);
	}

	if (bCameraZooming)
	{
		UpdateCameraZoom(DeltaTime);
	}
}

void UPlayerHandComponent::TriggerChop()
{
	if (!bIsAnimating && CurrentFingerToChop > 0)
	{
		ChopNextFinger();
	}
}

void UPlayerHandComponent::ChopNextFinger()
{
	if (CurrentFingerToChop <= 0 || bIsAnimating) return;

	CurrentKnife = GetKnifeMesh(CurrentFingerToChop);
	CurrentFinger = GetFingerMesh(CurrentFingerToChop);

	if (!CurrentKnife || !CurrentFinger)
	{
		UE_LOG(LogTemp, Warning, TEXT("Missing knife or finger mesh for finger %d"), CurrentFingerToChop);
		return;
	}

	// Only zoom camera for enemy hand (subtle dramatic effect)
	if (!bIsPlayerHand)
	{
		StartCameraZoomIn();
	}

	bIsAnimating = true;
	StartKnifeDown();
}

void UPlayerHandComponent::StartKnifeDown()
{
	AnimationPhase = 1;  // Knife coming down
	AnimationTimer = 0.0f;

	if (CurrentKnife)
	{
		KnifeAnimStartPos = CurrentKnife->GetRelativeLocation();
		KnifeReturnPos = KnifeAnimStartPos;  // Store where to return to
		KnifeAnimTargetPos = GetKnifeDownPos(CurrentFingerToChop);
		KnifeStartRot = CurrentKnife->GetRelativeRotation();
		KnifeTargetRot = KnifeStartRot;  // No rotation during down
	}
}

void UPlayerHandComponent::StartSlice()
{
	AnimationPhase = 3;  // Slicing
	AnimationTimer = 0.0f;

	if (CurrentKnife)
	{
		KnifeAnimStartPos = CurrentKnife->GetRelativeLocation();
		KnifeAnimTargetPos = GetKnifeSlicePos(CurrentFingerToChop);
		KnifeStartRot = CurrentKnife->GetRelativeRotation();
		// Add dramatic rotation during slice
		KnifeTargetRot = KnifeStartRot;
		KnifeTargetRot.Roll += KnifeSliceRotation;
		KnifeTargetRot.Pitch += FMath::RandRange(-5.0f, 5.0f);  // Slight random variation
	}
}

FVector UPlayerHandComponent::GetKnifeDownPos(int32 FingerIndex)
{
	switch (FingerIndex)
	{
		case 1: return Knife1DownPos;
		case 2: return Knife2DownPos;
		case 3: return Knife3DownPos;
		case 4: return Knife4DownPos;
		case 5: return Knife5DownPos;
		default: return Knife5DownPos;
	}
}

FVector UPlayerHandComponent::GetKnifeSlicePos(int32 FingerIndex)
{
	switch (FingerIndex)
	{
		case 1: return Knife1SlicePos;
		case 2: return Knife2SlicePos;
		case 3: return Knife3SlicePos;
		case 4: return Knife4SlicePos;
		case 5: return Knife5SlicePos;
		default: return Knife5SlicePos;
	}
}

void UPlayerHandComponent::FinishChop()
{
	// Camera shake on cut (for both hands)
	PlayCameraShake();

	// Spawn blood VFX at slice location
	SpawnBloodVFX();

	// Start camera zoom out (only for enemy hand)
	if (!bIsPlayerHand)
	{
		StartCameraZoomOut();
	}

	// Enable physics on the finger and apply impulse
	if (CurrentFinger)
	{
		CurrentFinger->SetSimulatePhysics(true);
		CurrentFinger->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		// Apply impulse - away from hand and slightly up
		FVector ImpulseDir = FVector(1.0f, 0.5f, 0.8f).GetSafeNormal();
		ImpulseDir += FVector(
			FMath::RandRange(-0.2f, 0.2f),
			FMath::RandRange(-0.2f, 0.2f),
			FMath::RandRange(0.0f, 0.3f)
		);
		CurrentFinger->AddImpulse(ImpulseDir * FingerImpulseStrength, NAME_None, true);

		// Add some spin
		FVector Torque = FVector(
			FMath::RandRange(-50.0f, 50.0f),
			FMath::RandRange(-50.0f, 50.0f),
			FMath::RandRange(-30.0f, 30.0f)
		);
		CurrentFinger->AddAngularImpulseInDegrees(Torque, NAME_None, true);
	}

	// Update state
	CurrentFingerToChop--;
	FingersRemaining--;

	UE_LOG(LogTemp, Log, TEXT("Finger chopped! Remaining: %d"), FingersRemaining);

	// Start knife return animation
	StartKnifeReturn();
}

void UPlayerHandComponent::StartKnifeReturn()
{
	AnimationPhase = 4;  // Returning up
	AnimationTimer = 0.0f;

	if (CurrentKnife)
	{
		KnifeAnimStartPos = CurrentKnife->GetRelativeLocation();
		KnifeAnimTargetPos = KnifeReturnPos;
		KnifeStartRot = CurrentKnife->GetRelativeRotation();
		KnifeTargetRot = FRotator::ZeroRotator;  // Reset rotation
	}
}

void UPlayerHandComponent::PlayCameraShake()
{
	bCameraShaking = true;
	CameraShakeTimer = 0.0f;
	ShakeOffset = FVector::ZeroVector;

	// Store original camera position if not zooming (for player hand)
	if (!bCameraZooming)
	{
		ADiceCamera* Cam = FindDiceCamera();
		if (Cam)
		{
			CameraZoomOriginalPos = Cam->GetActorLocation();
		}
	}
}

void UPlayerHandComponent::UpdateCameraShake(float DeltaTime)
{
	CameraShakeTimer += DeltaTime;

	if (CameraShakeTimer >= CamShakeDuration)
	{
		bCameraShaking = false;
		ShakeOffset = FVector::ZeroVector;

		// Reset camera to original position if not zooming
		if (!bCameraZooming && !CameraZoomOriginalPos.IsZero())
		{
			ADiceCamera* Cam = FindDiceCamera();
			if (Cam)
			{
				Cam->SetActorLocation(CameraZoomOriginalPos);
			}
			CameraZoomOriginalPos = FVector::ZeroVector;
		}
		return;
	}

	// Decay intensity over time with initial punch
	float Progress = CameraShakeTimer / CamShakeDuration;
	float CurrentIntensity = CamShakeIntensity * (1.0f - Progress * Progress);  // Quadratic falloff

	// Calculate shake offset (will be applied in UpdateCameraZoom or directly)
	ShakeOffset = FVector(
		FMath::RandRange(-CurrentIntensity, CurrentIntensity),
		FMath::RandRange(-CurrentIntensity, CurrentIntensity),
		FMath::RandRange(-CurrentIntensity * 0.5f, CurrentIntensity * 0.5f)
	);

	// If not zooming, apply shake directly to DiceCamera
	if (!bCameraZooming)
	{
		ADiceCamera* Cam = FindDiceCamera();
		if (Cam)
		{
			FVector BasePos = CameraZoomOriginalPos.IsZero() ? Cam->GetActorLocation() : CameraZoomOriginalPos;
			Cam->SetActorLocation(BasePos + ShakeOffset);
		}
	}
	// If zooming, the shake offset will be applied in UpdateCameraZoom
}

void UPlayerHandComponent::UpdateAnimation(float DeltaTime)
{
	switch (AnimationPhase)
	{
		case 1:  // Knife coming down
		{
			AnimationTimer += DeltaTime * KnifeDownSpeed;
			float Alpha = FMath::Clamp(AnimationTimer, 0.0f, 1.0f);

			// Ease out for impact feel
			float EasedAlpha = 1.0f - FMath::Pow(1.0f - Alpha, 3.0f);

			if (CurrentKnife)
			{
				FVector NewPos = FMath::Lerp(KnifeAnimStartPos, KnifeAnimTargetPos, EasedAlpha);
				CurrentKnife->SetRelativeLocation(NewPos);

				// Slight menacing wobble as it comes down
				FRotator WobbleRot = KnifeStartRot;
				float WobbleIntensity = (1.0f - Alpha) * 3.0f;  // Decreases as it gets closer
				WobbleRot.Roll += FMath::Sin(Alpha * PI * 4.0f) * WobbleIntensity;
				WobbleRot.Pitch += FMath::Sin(Alpha * PI * 3.0f) * WobbleIntensity * 0.5f;
				CurrentKnife->SetRelativeRotation(WobbleRot);
			}

			if (Alpha >= 1.0f)
			{
				// Start waiting
				AnimationPhase = 2;
				AnimationTimer = 0.0f;
			}
			break;
		}

		case 2:  // Waiting before slice
		{
			AnimationTimer += DeltaTime;
			if (AnimationTimer >= WaitTimeBeforeSlice)
			{
				StartSlice();
			}
			break;
		}

		case 3:  // Slicing
		{
			AnimationTimer += DeltaTime * KnifeSliceSpeed;
			float Alpha = FMath::Clamp(AnimationTimer, 0.0f, 1.0f);

			// Ease out for impactful slice
			float EasedAlpha = 1.0f - FMath::Pow(1.0f - Alpha, 2.0f);

			if (CurrentKnife)
			{
				// Position
				FVector NewPos = FMath::Lerp(KnifeAnimStartPos, KnifeAnimTargetPos, EasedAlpha);
				CurrentKnife->SetRelativeLocation(NewPos);

				// Rotation - dramatic twist during slice
				FRotator NewRot = FMath::Lerp(KnifeStartRot, KnifeTargetRot, EasedAlpha);
				// Add a slight wobble for impact feel
				NewRot.Yaw += FMath::Sin(Alpha * PI * 2.0f) * 3.0f * (1.0f - Alpha);
				CurrentKnife->SetRelativeRotation(NewRot);
			}

			if (Alpha >= 1.0f)
			{
				FinishChop();
			}
			break;
		}

		case 4:  // Knife returning up slowly
		{
			AnimationTimer += DeltaTime * KnifeReturnSpeed;
			float Alpha = FMath::Clamp(AnimationTimer, 0.0f, 1.0f);

			// Smooth ease in-out for gentle return
			float EasedAlpha = Alpha < 0.5f
				? 2.0f * Alpha * Alpha
				: 1.0f - FMath::Pow(-2.0f * Alpha + 2.0f, 2.0f) / 2.0f;

			if (CurrentKnife)
			{
				// Position
				FVector NewPos = FMath::Lerp(KnifeAnimStartPos, KnifeAnimTargetPos, EasedAlpha);
				CurrentKnife->SetRelativeLocation(NewPos);

				// Rotation - smoothly return to neutral
				FRotator NewRot = FMath::Lerp(KnifeStartRot, KnifeTargetRot, EasedAlpha);
				CurrentKnife->SetRelativeRotation(NewRot);
			}

			if (Alpha >= 1.0f)
			{
				// Animation complete
				bIsAnimating = false;
				AnimationPhase = 0;
				CurrentKnife = nullptr;
				CurrentFinger = nullptr;

				// Broadcast chop complete event
				OnChopComplete.Broadcast();
			}
			break;
		}

		default:
			break;
	}
}

UStaticMeshComponent* UPlayerHandComponent::GetFingerMesh(int32 FingerIndex)
{
	switch (FingerIndex)
	{
		case 1: return Finger1;
		case 2: return Finger2;
		case 3: return Finger3;
		case 4: return Finger4;
		case 5: return Finger5;
		default: return nullptr;
	}
}

UStaticMeshComponent* UPlayerHandComponent::GetKnifeMesh(int32 FingerIndex)
{
	switch (FingerIndex)
	{
		case 1: return Knife1;
		case 2: return Knife2;
		case 3: return Knife3;
		case 4: return Knife4;
		case 5: return Knife5;
		default: return nullptr;
	}
}

void UPlayerHandComponent::SpawnBloodVFX()
{
	if (!CurrentKnife) return;

	// Get slice position (where the knife is)
	FVector SlicePos = CurrentKnife->GetComponentLocation();

	// Spawn main blood burst at slice point
	if (BloodParticleSystem)
	{
		// Spawn at knife position, pointing down for drip effect
		FRotator ParticleRot = FRotator(-90.0f, 0, 0);

		UNiagaraComponent* BloodFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			BloodParticleSystem,
			SlicePos,
			ParticleRot,
			FVector(1.0f),
			true,
			true,
			ENCPoolMethod::None,
			true
		);

		if (BloodFX)
		{
			// Set color parameters - try common naming conventions
			BloodFX->SetColorParameter(TEXT("Color"), BloodColor);
			BloodFX->SetColorParameter(TEXT("BloodColor"), BloodColor);
			BloodFX->SetColorParameter(TEXT("ParticleColor"), BloodColor);
		}

		// Spawn splatter particles around the slice
		for (int32 i = 0; i < BloodSplatterCount; i++)
		{
			FVector SplatterOffset = FVector(
				FMath::RandRange(-BloodSplatterSpread, BloodSplatterSpread),
				FMath::RandRange(-BloodSplatterSpread, BloodSplatterSpread),
				FMath::RandRange(-5.0f, 5.0f)
			);

			FVector SplatterPos = SlicePos + SplatterOffset;
			float SplatterScale = FMath::RandRange(0.3f, 0.7f);
			FRotator SplatterRot = FRotator(
				FMath::RandRange(-45.0f, 45.0f),
				FMath::RandRange(0.0f, 360.0f),
				0
			);

			UNiagaraComponent* SplatterFX = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				BloodParticleSystem,
				SplatterPos,
				SplatterRot,
				FVector(SplatterScale),
				true,
				true,
				ENCPoolMethod::None,
				true
			);

			if (SplatterFX)
			{
				SplatterFX->SetColorParameter(TEXT("Color"), BloodColor);
				SplatterFX->SetColorParameter(TEXT("BloodColor"), BloodColor);
				SplatterFX->SetColorParameter(TEXT("ParticleColor"), BloodColor);
			}
		}
	}
}

void UPlayerHandComponent::StartCameraZoomIn()
{
	ADiceCamera* Cam = FindDiceCamera();
	if (Cam)
	{
		bCameraZooming = true;
		bCameraZoomingOut = false;
		CameraZoomProgress = 0.0f;
		CameraZoomStartPos = Cam->GetActorLocation();
		CameraZoomStartRot = Cam->GetActorRotation();
		CameraZoomOriginalPos = CameraZoomStartPos;
		CameraZoomOriginalRot = CameraZoomStartRot;

		// Zoom toward the hand
		FVector HandPos = GetOwner()->GetActorLocation();
		FVector ToHand = (HandPos - CameraZoomStartPos).GetSafeNormal();
		CameraZoomTargetPos = CameraZoomStartPos + ToHand * ChopCameraZoomAmount;

		// Rotate to look at the hand (subtle)
		FVector LookDir = (HandPos - CameraZoomTargetPos).GetSafeNormal();
		CameraZoomTargetRot = LookDir.Rotation();
		// Blend with original rotation for subtlety
		CameraZoomTargetRot = FMath::Lerp(CameraZoomStartRot, CameraZoomTargetRot, 0.3f);
	}
}

void UPlayerHandComponent::StartCameraZoomOut()
{
	if (bCameraZooming && !bCameraZoomingOut)
	{
		// Reverse the zoom
		bCameraZoomingOut = true;
		CameraZoomProgress = 0.0f;

		ADiceCamera* Cam = FindDiceCamera();
		if (Cam)
		{
			// Swap start and target - go back to original position and rotation
			CameraZoomStartPos = Cam->GetActorLocation();
			CameraZoomStartRot = Cam->GetActorRotation();
			CameraZoomTargetPos = CameraZoomOriginalPos;
			CameraZoomTargetRot = CameraZoomOriginalRot;
		}
	}
}

void UPlayerHandComponent::UpdateCameraZoom(float DeltaTime)
{
	CameraZoomProgress += DeltaTime * ChopCameraZoomSpeed;
	float Alpha = FMath::Clamp(CameraZoomProgress, 0.0f, 1.0f);

	// Smooth easing
	float EasedAlpha;
	if (bCameraZoomingOut)
	{
		// Ease out for smooth return
		EasedAlpha = 1.0f - FMath::Pow(1.0f - Alpha, 2.0f);
	}
	else
	{
		// Ease in-out for dramatic zoom in
		EasedAlpha = Alpha < 0.5f
			? 2.0f * Alpha * Alpha
			: 1.0f - FMath::Pow(-2.0f * Alpha + 2.0f, 2.0f) / 2.0f;
	}

	ADiceCamera* Cam = FindDiceCamera();
	if (Cam)
	{
		FVector NewPos = FMath::Lerp(CameraZoomStartPos, CameraZoomTargetPos, EasedAlpha);
		FRotator NewRot = FMath::Lerp(CameraZoomStartRot, CameraZoomTargetRot, EasedAlpha);

		// Apply shake offset on top of zoom position
		if (bCameraShaking)
		{
			NewPos += ShakeOffset;
		}

		Cam->SetActorLocation(NewPos);
		Cam->SetActorRotation(NewRot);
	}

	if (Alpha >= 1.0f)
	{
		if (bCameraZoomingOut)
		{
			bCameraZooming = false;
			CameraZoomOriginalPos = FVector::ZeroVector;
			CameraZoomOriginalRot = FRotator::ZeroRotator;
		}
		// If zooming in, wait for zoom out to be triggered
	}
}

ADiceCamera* UPlayerHandComponent::FindDiceCamera()
{
	if (CachedDiceCamera && IsValid(CachedDiceCamera))
	{
		return CachedDiceCamera;
	}

	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADiceCamera::StaticClass(), FoundCameras);
	for (AActor* A : FoundCameras)
	{
		ADiceCamera* Cam = Cast<ADiceCamera>(A);
		if (Cam && Cam->bIsMainCamera)
		{
			CachedDiceCamera = Cam;
			return Cam;
		}
	}

	// If no main camera found, use the first one
	if (FoundCameras.Num() > 0)
	{
		CachedDiceCamera = Cast<ADiceCamera>(FoundCameras[0]);
		return CachedDiceCamera;
	}

	return nullptr;
}
