#include "Dice.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
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
	Mesh->SetCollisionObjectType(ECC_PhysicsBody);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	bHasBeenThrown = false;
	bHasPlayedLandSound = false;
	bIsHighlighted = false;
	bIsMatched = false;
	bIsBeingDragged = false;
	bHasGlow = false;
	CurrentValue = 0;
	HighlightPulse = 0.0f;
	TextColor = FColor::White;
	MeshNormalizeScale = 1.0f;
	bHighlightRotSet = false;
	BaseHighlightRot = FRotator::ZeroRotator;
	BaseHighlightPos = FVector::ZeroVector;

	// Text defaults
	FaceTextSize = 50.0f;
	FaceTextOffset = 51.0f;

	SetupFaceTexts();
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

	float BaseScale = DiceSize * MeshNormalizeScale;
	Mesh->SetWorldScale3D(FVector(BaseScale));

	// Don't apply hover effect to matched or dragged dice
	if (bIsMatched || bIsBeingDragged)
	{
		if (bHighlightRotSet)
		{
			bHighlightRotSet = false;
			HighlightPulse = 0.0f;
			// Reset scale when exiting highlight
			Mesh->SetWorldScale3D(FVector(BaseScale));
		}
		return;
	}

	if (bIsHighlighted)
	{
		// Store base position and rotation when first highlighted
		if (!bHighlightRotSet)
		{
			BaseHighlightRot = GetActorRotation();
			BaseHighlightPos = GetActorLocation();
			bHighlightRotSet = true;
			HighlightPulse = 0.0f;
		}

		HighlightPulse += DeltaTime;

		// Ease-in for smooth rise (first 0.3 seconds)
		float RiseAlpha = FMath::Min(HighlightPulse / 0.3f, 1.0f);
		// Smooth ease out cubic
		float EasedRise = 1.0f - FMath::Pow(1.0f - RiseAlpha, 3.0f);

		// Base float height with smooth ease-in
		float BaseFloatHeight = 12.0f * EasedRise;
		// Juicy bob - multiple sine waves for organic feel
		float BobHeight = FMath::Sin(HighlightPulse * 2.5f) * 4.0f + FMath::Sin(HighlightPulse * 4.0f) * 1.5f;

		FVector FloatPos = BaseHighlightPos;
		FloatPos.Z += BaseFloatHeight + BobHeight * EasedRise;
		SetActorLocation(FloatPos);

		// Juicy sway rotation - layered sine waves for organic movement
		FRotator SwayRot = BaseHighlightRot;
		float SwayIntensity = EasedRise;  // Ease sway in too
		SwayRot.Roll += (FMath::Sin(HighlightPulse * 1.8f) * 10.0f + FMath::Sin(HighlightPulse * 3.2f) * 4.0f) * SwayIntensity;
		SwayRot.Pitch += (FMath::Sin(HighlightPulse * 1.3f) * 7.0f + FMath::Sin(HighlightPulse * 2.7f) * 3.0f) * SwayIntensity;
		SwayRot.Yaw += FMath::Sin(HighlightPulse * 0.8f) * 5.0f * SwayIntensity;  // Slow yaw drift
		SetActorRotation(SwayRot);

		// Subtle scale pulse for "breathing" effect
		float BreathScale = 1.0f + FMath::Sin(HighlightPulse * 2.0f) * 0.03f * EasedRise;
		Mesh->SetWorldScale3D(FVector(BaseScale * BreathScale));
	}
	else
	{
		// Reset position and rotation when not highlighted
		if (bHighlightRotSet)
		{
			SetActorLocation(BaseHighlightPos);
			SetActorRotation(BaseHighlightRot);
			bHighlightRotSet = false;
			HighlightPulse = 0.0f;
			// Reset scale back to normal
			Mesh->SetWorldScale3D(FVector(BaseScale));
		}
	}
}

void ADice::Throw(FVector Direction, float Force)
{
	bHasBeenThrown = true;
	bHasPlayedLandSound = false;  // Reset so landing sound plays again

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

	FColor DiceColor = FColor::White;
	if (bIsMatched)
	{
		DiceColor = FColor::Green;
	}
	else if (bIsHighlighted)
	{
		DiceColor = FColor::Yellow;
	}

	for (int32 i = 1; i <= 6; i++)
	{
		FVector LocalCenter = GetFaceNormal(i) * CubeExtent;
		FVector WorldCenter = GetActorLocation() + GetActorRotation().RotateVector(LocalCenter);

		FColor Color = (i == TopFace) ? DiceColor : FColor(100, 100, 100);
		FString Num = FString::FromInt(i);

		DrawDebugString(GetWorld(), WorldCenter, Num, nullptr, Color, 0.0f, true, 2.0f);
	}

	FVector TopIndicator = GetActorLocation() + FVector(0, 0, CubeExtent + 15.0f);
	int32 DisplayValue = (CurrentValue > 0) ? CurrentValue : TopFace;
	DrawDebugString(GetWorld(), TopIndicator, FString::Printf(TEXT("[%d]"), DisplayValue), nullptr, DiceColor, 0.0f, true, 3.5f);
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

void ADice::SetupFaceTexts()
{
	for (int32 i = 1; i <= 6; i++)
	{
		FString CompName = FString::Printf(TEXT("FaceText_%d"), i);
		UTextRenderComponent* TextComp = CreateDefaultSubobject<UTextRenderComponent>(*CompName);
		TextComp->SetupAttachment(Mesh);

		FVector LocalPos = GetFaceNormal(i) * FaceTextOffset;
		TextComp->SetRelativeLocation(LocalPos);
		TextComp->SetRelativeRotation(GetFaceTextRotation(i));

		TextComp->SetText(FText::FromString(FString::FromInt(i)));
		TextComp->SetHorizontalAlignment(EHTA_Center);
		TextComp->SetVerticalAlignment(EVRTA_TextCenter);
		TextComp->SetWorldSize(FaceTextSize);
		TextComp->SetTextRenderColor(FColor::White);

		FaceTexts.Add(TextComp);
	}
}

FRotator ADice::GetFaceTextRotation(int32 FaceIndex)
{
	switch (FaceIndex)
	{
		case 1: return FRotator(90, 0, 180);
		case 2: return FRotator(0, 0, 90);
		case 3: return FRotator(0, -90, 0);
		case 4: return FRotator(0, 90, 0);
		case 5: return FRotator(0, 180, 90);   // Flipped both ways
		case 6: return FRotator(-90, 0, 0);
		default: return FRotator::ZeroRotator;
	}
}

void ADice::SetValue(int32 NewValue)
{
	CurrentValue = FMath::Clamp(NewValue, 1, 6);
}

void ADice::SetHighlighted(bool bHighlight)
{
	// Reset position and rotation when unhighlighting
	if (!bHighlight && bHighlightRotSet)
	{
		SetActorLocation(BaseHighlightPos);
		SetActorRotation(BaseHighlightRot);
		bHighlightRotSet = false;
	}

	bIsHighlighted = bHighlight;
	if (!bHighlight)
	{
		HighlightPulse = 0.0f;
	}
}

void ADice::SetMatched(bool bMatch)
{
	bIsMatched = bMatch;
	if (bMatch)
	{
		bIsHighlighted = false;
	}
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

void ADice::SetCustomMesh(UStaticMesh* NewMesh, float MeshScale)
{
	if (NewMesh && Mesh)
	{
		Mesh->SetStaticMesh(NewMesh);
		MeshNormalizeScale = MeshScale;
	}
}

void ADice::SetCustomMaterial(UMaterialInterface* NewMaterial)
{
	if (NewMaterial && Mesh)
	{
		Mesh->SetMaterial(0, NewMaterial);
	}
}

void ADice::SetTextColor(FColor NewColor)
{
	TextColor = NewColor;
	for (UTextRenderComponent* TextComp : FaceTexts)
	{
		if (TextComp)
		{
			TextComp->SetTextRenderColor(NewColor);
		}
	}
}

void ADice::SetGlowEnabled(bool bEnable)
{
	bHasGlow = bEnable;
	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(bEnable);
		Mesh->SetCustomDepthStencilValue(bEnable ? 1 : 0);
	}
}

void ADice::SetFaceNumbersVisible(bool bVisible)
{
	for (UTextRenderComponent* TextComp : FaceTexts)
	{
		if (TextComp)
		{
			TextComp->SetVisibility(bVisible);
		}
	}
}

void ADice::SetAllFacesText(const FString& Text)
{
	for (int32 i = 0; i < FaceTexts.Num(); i++)
	{
		UTextRenderComponent* TextComp = FaceTexts[i];
		if (TextComp)
		{
			TextComp->SetText(FText::FromString(Text));
			TextComp->SetWorldSize(FaceTextSize * 1.2f);  // Larger for number display
			// Push text further out so it's visible
			FVector LocalPos = GetFaceNormal(i + 1) * (FaceTextOffset + 5.0f);
			TextComp->SetRelativeLocation(LocalPos);
			TextComp->SetVisibility(true);
		}
	}
}

void ADice::SetTextSettings(float Size, float Offset)
{
	FaceTextSize = Size;
	FaceTextOffset = Offset;

	// Update existing text components
	for (int32 i = 0; i < FaceTexts.Num(); i++)
	{
		UTextRenderComponent* TextComp = FaceTexts[i];
		if (TextComp)
		{
			// Update position based on new offset
			FVector LocalPos = GetFaceNormal(i + 1) * FaceTextOffset;
			TextComp->SetRelativeLocation(LocalPos);

			// Update size
			TextComp->SetWorldSize(FaceTextSize);
		}
	}
}
