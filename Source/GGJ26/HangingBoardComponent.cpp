#include "HangingBoardComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"

UHangingBoardComponent::UHangingBoardComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	ContentLabelName = TEXT("ContentLabel");

	// Default positions
	HiddenOffset = FVector(0.0f, 0.0f, 300.0f);  // 300 units above
	TargetPosition = FVector(850.0f, 1343.5f, 229.4f);

	DescendSpeed = 0.8f;  // Slower descent
	AscendSpeed = 2.0f;
	ShimmerAmount = 3.0f;
	ShimmerSpeed = 4.0f;

	BoardText = TEXT("MASQUERADE?");
	TextRevealSpeed = 12.0f;
	DelayBeforeButton = 3.0f;  // 3 second delay to read

	CurrentState = EBoardState::Hidden;
	AnimProgress = 0.0f;
	ShimmerTimer = 0.0f;

	ContentLabel = nullptr;
	BoardMesh = nullptr;
	RevealedChars = 0;
	TextRevealTimer = 0.0f;
	bTextRevealing = false;
	ButtonDelayTimer = 0.0f;
	bWaitingForButtonDelay = false;
}

void UHangingBoardComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		// Find content label
		TArray<UTextRenderComponent*> TextComponents;
		Owner->GetComponents<UTextRenderComponent>(TextComponents);

		for (UTextRenderComponent* Text : TextComponents)
		{
			if (Text->GetName().Contains(ContentLabelName))
			{
				ContentLabel = Text;
				break;
			}
		}

		if (!ContentLabel && TextComponents.Num() > 0)
		{
			ContentLabel = TextComponents[0];
		}

		// Find board mesh (first static mesh)
		TArray<UStaticMeshComponent*> Meshes;
		Owner->GetComponents<UStaticMeshComponent>(Meshes);
		if (Meshes.Num() > 0)
		{
			BoardMesh = Meshes[0];
			// Hide mesh initially to save on shadows
			BoardMesh->SetVisibility(false);
			BoardMesh->SetCastShadow(false);
		}

		// Clear text initially
		if (ContentLabel)
		{
			ContentLabel->SetText(FText::FromString(TEXT("")));
			ContentLabel->SetVisibility(false);
		}
	}

	// Set initial hidden position
	HiddenPosition = TargetPosition + HiddenOffset;
	if (Owner)
	{
		Owner->SetActorLocation(HiddenPosition);
	}
}

void UHangingBoardComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	switch (CurrentState)
	{
		case EBoardState::Descending:
			UpdateDescend(DeltaTime);
			break;

		case EBoardState::Visible:
			UpdateShimmer(DeltaTime);
			break;

		case EBoardState::Ascending:
			UpdateAscend(DeltaTime);
			break;

		default:
			break;
	}

	// Update text reveal
	if (bTextRevealing)
	{
		UpdateTextReveal(DeltaTime);
	}

	// Handle delay before activating button
	if (bWaitingForButtonDelay)
	{
		ButtonDelayTimer += DeltaTime;
		if (ButtonDelayTimer >= DelayBeforeButton)
		{
			bWaitingForButtonDelay = false;
			OnBoardArrived.Broadcast();
		}
	}
}

void UHangingBoardComponent::ShowBoard(const FString& Text)
{
	if (Text.Len() > 0)
	{
		BoardText = Text;
	}

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Enable mesh visibility
	if (BoardMesh)
	{
		BoardMesh->SetVisibility(true);
	}
	if (ContentLabel)
	{
		ContentLabel->SetVisibility(true);
	}

	// Setup animation
	HiddenPosition = TargetPosition + HiddenOffset;
	StartPosition = HiddenPosition;
	Owner->SetActorLocation(HiddenPosition);

	AnimProgress = 0.0f;
	CurrentState = EBoardState::Descending;

	// Clear text, will reveal after board arrives
	if (ContentLabel)
	{
		ContentLabel->SetText(FText::FromString(TEXT("")));
	}
	bTextRevealing = false;
	RevealedChars = 0;
	bWaitingForButtonDelay = false;
	ButtonDelayTimer = 0.0f;
}

void UHangingBoardComponent::HideBoard()
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	StartPosition = Owner->GetActorLocation();
	AnimProgress = 0.0f;
	CurrentState = EBoardState::Ascending;
	bTextRevealing = false;
}

void UHangingBoardComponent::SetBoardText(const FString& Text)
{
	BoardText = Text;
	if (ContentLabel && CurrentState == EBoardState::Visible)
	{
		// Start text reveal
		FullText = Text;
		RevealedChars = 0;
		TextRevealTimer = 0.0f;
		bTextRevealing = true;
	}
}

void UHangingBoardComponent::UpdateDescend(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	AnimProgress += DeltaTime * DescendSpeed;

	if (AnimProgress >= 1.0f)
	{
		AnimProgress = 1.0f;
		Owner->SetActorLocation(TargetPosition);
		CurrentState = EBoardState::Visible;
		ShimmerTimer = 0.0f;

		// Start text reveal
		FullText = BoardText;
		RevealedChars = 0;
		TextRevealTimer = 0.0f;
		bTextRevealing = true;

		// Start delay before button activation (gives time to read)
		bWaitingForButtonDelay = true;
		ButtonDelayTimer = 0.0f;
	}
	else
	{
		// Bouncy easing
		float T = EaseOutBounce(AnimProgress);
		FVector NewPos = FMath::Lerp(StartPosition, TargetPosition, T);

		// Add swing motion during descent
		float Swing = FMath::Sin(AnimProgress * 8.0f * PI) * (1.0f - AnimProgress) * 15.0f;
		NewPos.Y += Swing;

		Owner->SetActorLocation(NewPos);

		// Slight rotation swing
		FRotator CurrentRot = Owner->GetActorRotation();
		CurrentRot.Roll = FMath::Sin(AnimProgress * 6.0f * PI) * (1.0f - AnimProgress) * 10.0f;
		Owner->SetActorRotation(CurrentRot);
	}
}

void UHangingBoardComponent::UpdateAscend(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	AnimProgress += DeltaTime * AscendSpeed;

	if (AnimProgress >= 1.0f)
	{
		AnimProgress = 1.0f;
		Owner->SetActorLocation(HiddenPosition);
		CurrentState = EBoardState::Hidden;

		// Reset rotation
		FRotator CurrentRot = Owner->GetActorRotation();
		CurrentRot.Roll = 0.0f;
		Owner->SetActorRotation(CurrentRot);

		// Hide mesh to save on shadows
		if (BoardMesh)
		{
			BoardMesh->SetVisibility(false);
		}
		if (ContentLabel)
		{
			ContentLabel->SetVisibility(false);
		}

		OnBoardRetracted.Broadcast();
	}
	else
	{
		// Ease in back (swoosh up)
		float T = EaseInBack(AnimProgress);
		FVector NewPos = FMath::Lerp(StartPosition, HiddenPosition, T);
		Owner->SetActorLocation(NewPos);
	}
}

void UHangingBoardComponent::UpdateShimmer(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	ShimmerTimer += DeltaTime;

	// Gentle swaying motion
	float SwayX = FMath::Sin(ShimmerTimer * ShimmerSpeed * 0.7f) * ShimmerAmount * 0.3f;
	float SwayY = FMath::Sin(ShimmerTimer * ShimmerSpeed) * ShimmerAmount;
	float SwayZ = FMath::Sin(ShimmerTimer * ShimmerSpeed * 1.3f) * ShimmerAmount * 0.2f;

	FVector ShimmerPos = TargetPosition + FVector(SwayX, SwayY, SwayZ);
	Owner->SetActorLocation(ShimmerPos);

	// Gentle rotation sway
	FRotator CurrentRot = Owner->GetActorRotation();
	CurrentRot.Roll = FMath::Sin(ShimmerTimer * ShimmerSpeed * 0.8f) * 2.0f;
	CurrentRot.Pitch = FMath::Sin(ShimmerTimer * ShimmerSpeed * 0.5f) * 1.0f;
	Owner->SetActorRotation(CurrentRot);
}

void UHangingBoardComponent::UpdateTextReveal(float DeltaTime)
{
	if (!ContentLabel) return;

	TextRevealTimer += DeltaTime * TextRevealSpeed;
	int32 NewCharCount = FMath::FloorToInt(TextRevealTimer);

	if (NewCharCount > RevealedChars)
	{
		RevealedChars = NewCharCount;

		if (RevealedChars >= FullText.Len())
		{
			// Fully revealed
			ContentLabel->SetText(FText::FromString(FullText));
			bTextRevealing = false;
		}
		else
		{
			// Partial reveal with scrambled remaining chars
			FString DisplayText;
			TArray<TCHAR> ScrambleChars = { '#', '@', '$', '%', '&', '*', '?', '!' };

			for (int32 i = 0; i < FullText.Len(); i++)
			{
				if (i < RevealedChars)
				{
					DisplayText.AppendChar(FullText[i]);
				}
				else
				{
					int32 RandIdx = FMath::RandRange(0, ScrambleChars.Num() - 1);
					DisplayText.AppendChar(ScrambleChars[RandIdx]);
				}
			}
			ContentLabel->SetText(FText::FromString(DisplayText));
		}
	}
}

float UHangingBoardComponent::EaseOutBounce(float t)
{
	if (t < 1.0f / 2.75f)
	{
		return 7.5625f * t * t;
	}
	else if (t < 2.0f / 2.75f)
	{
		t -= 1.5f / 2.75f;
		return 7.5625f * t * t + 0.75f;
	}
	else if (t < 2.5f / 2.75f)
	{
		t -= 2.25f / 2.75f;
		return 7.5625f * t * t + 0.9375f;
	}
	else
	{
		t -= 2.625f / 2.75f;
		return 7.5625f * t * t + 0.984375f;
	}
}

float UHangingBoardComponent::EaseInBack(float t)
{
	float c1 = 1.70158f;
	float c3 = c1 + 1.0f;
	return c3 * t * t * t - c1 * t * t;
}
