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
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Overlap);

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

	BaseColor = FLinearColor(0.2f, 0.2f, 0.3f, 1.0f);
	HighlightColor = FLinearColor(0.4f, 0.8f, 0.4f, 1.0f);
	UsedColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.5f);
}

void ADiceModifier::BeginPlay()
{
	Super::BeginPlay();
	UpdateVisuals();
}

void ADiceModifier::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsHighlighted && !bIsUsed)
	{
		float Pulse = (FMath::Sin(GetWorld()->GetTimeSeconds() * 4.0f) + 1.0f) * 0.5f;
		FVector NewScale = FVector(0.5f + Pulse * 0.05f, 0.5f + Pulse * 0.05f, 1.0f);
		PlaneMesh->SetWorldScale3D(NewScale);
	}
}

void ADiceModifier::SetHighlighted(bool bHighlight)
{
	if (bIsUsed) return;

	bIsHighlighted = bHighlight;
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

	if (bIsUsed)
	{
		PlaneMesh->SetVisibility(false);
		ModifierText->SetTextRenderColor(FColor(50, 50, 50, 128));
	}
	else if (bIsHighlighted)
	{
		ModifierText->SetTextRenderColor(FColor::Green);
	}
	else
	{
		ModifierText->SetTextRenderColor(FColor::White);
	}
}
