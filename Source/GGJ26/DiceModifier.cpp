#include "DiceModifier.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"

ADiceModifier::ADiceModifier()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(Root);
	CollisionBox->SetBoxExtent(FVector(25.0f, 25.0f, 5.0f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaneMesh"));
	PlaneMesh->SetupAttachment(Root);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshAsset(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshAsset.Succeeded())
	{
		PlaneMesh->SetStaticMesh(PlaneMeshAsset.Object);
	}
	PlaneMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 1.0f));
	PlaneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ModifierText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ModifierText"));
	ModifierText->SetupAttachment(Root);
	ModifierText->SetRelativeLocation(FVector(0, 0, 10.0f));
	ModifierText->SetRelativeRotation(FRotator(90, 0, 180));
	ModifierText->SetHorizontalAlignment(EHTA_Center);
	ModifierText->SetVerticalAlignment(EVRTA_TextCenter);
	ModifierText->SetWorldSize(20.0f);
	ModifierText->SetTextRenderColor(FColor::White);

	ModifierType = EModifierType::PlusOne;
	bIsUsed = false;
	bIsHighlighted = false;
	bIsActive = false;

	BaseColor = FLinearColor(0.2f, 0.2f, 0.3f, 1.0f);
	HighlightColor = FLinearColor(0.4f, 0.8f, 0.4f, 1.0f);
	UsedColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.5f);
	InactiveColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);

	ActivationProgress = 0.0f;
	bActivating = false;
}

void ADiceModifier::BeginPlay()
{
	Super::BeginPlay();
	// Hide the plane mesh - only show text
	if (PlaneMesh)
	{
		PlaneMesh->SetVisibility(false);
	}
	UpdateVisuals();
}

void ADiceModifier::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bActivating)
	{
		ActivationProgress += DeltaTime * 3.0f;
		if (ActivationProgress >= 1.0f)
		{
			ActivationProgress = 1.0f;
			bActivating = false;
		}

		float BounceScale = 0.5f + FMath::Sin(ActivationProgress * PI) * 0.15f;
		PlaneMesh->SetWorldScale3D(FVector(BounceScale, BounceScale, 1.0f));

		float PopUp = FMath::Sin(ActivationProgress * PI) * 10.0f;
		FVector Loc = PlaneMesh->GetRelativeLocation();
		Loc.Z = PopUp;
		PlaneMesh->SetRelativeLocation(Loc);

		UpdateVisuals();
	}
	else if (bIsHighlighted && !bIsUsed && bIsActive)
	{
		float Pulse = (FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) + 1.0f) * 0.5f;
		FVector NewScale = FVector(0.5f + Pulse * 0.05f, 0.5f + Pulse * 0.05f, 1.0f);
		PlaneMesh->SetWorldScale3D(NewScale);
	}
}

void ADiceModifier::SetHighlighted(bool bHighlight)
{
	if (bIsUsed || !bIsActive) return;

	bIsHighlighted = bHighlight;
	UpdateVisuals();
}

void ADiceModifier::SetActive(bool bActive)
{
	if (bIsUsed) return;

	if (bActive && !bIsActive)
	{
		bActivating = true;
		ActivationProgress = 0.0f;
	}

	bIsActive = bActive;
	UpdateVisuals();
}

void ADiceModifier::UseModifier()
{
	bIsUsed = true;
	bIsHighlighted = false;
	UpdateVisuals();
}

int32 ADiceModifier::ApplyToValue(int32 OriginalValue)
{
	switch (ModifierType)
	{
		case EModifierType::MinusOne:
			return FMath::Max(1, OriginalValue - 1);
		case EModifierType::PlusOne:
			return FMath::Min(6, OriginalValue + 1);
		case EModifierType::PlusTwo:
			return FMath::Min(6, OriginalValue + 2);
		case EModifierType::Flip:
			return 7 - OriginalValue;
		default:
			return OriginalValue;
	}
}

FString ADiceModifier::GetModifierDisplayText()
{
	switch (ModifierType)
	{
		case EModifierType::MinusOne: return TEXT("-1");
		case EModifierType::PlusOne: return TEXT("+1");
		case EModifierType::PlusTwo: return TEXT("+2");
		case EModifierType::Flip: return TEXT("FLIP");
		case EModifierType::RerollOne: return TEXT("RE:1");
		case EModifierType::RerollAll: return TEXT("RE:ALL");
		default: return TEXT("?");
	}
}

bool ADiceModifier::RequiresDiceSelection()
{
	return ModifierType == EModifierType::MinusOne ||
		   ModifierType == EModifierType::PlusOne ||
		   ModifierType == EModifierType::PlusTwo ||
		   ModifierType == EModifierType::Flip ||
		   ModifierType == EModifierType::RerollOne;
}

void ADiceModifier::UpdateVisuals()
{
	ModifierText->SetText(FText::FromString(GetModifierDisplayText()));

	// Always keep plane hidden
	if (PlaneMesh)
	{
		PlaneMesh->SetVisibility(false);
	}

	if (bIsUsed)
	{
		ModifierText->SetTextRenderColor(FColor(50, 50, 50, 128));
	}
	else if (!bIsActive)
	{
		float Grey = 80;
		ModifierText->SetTextRenderColor(FColor(Grey, Grey, Grey, 255));
	}
	else if (bIsHighlighted)
	{
		ModifierText->SetTextRenderColor(FColor::Green);
	}
	else
	{
		float Lerp = bActivating ? ActivationProgress : 1.0f;
		uint8 ColorVal = FMath::Lerp(80, 255, Lerp);
		ModifierText->SetTextRenderColor(FColor(ColorVal, ColorVal, ColorVal, 255));
	}
}
