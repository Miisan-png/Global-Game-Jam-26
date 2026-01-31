#include "IRButtonComponent.h"
#include "DiceCamera.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

UIRButtonComponent::UIRButtonComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Default to Yes button
	ButtonType = EIRButtonType::Yes;

	// Mesh names
	SwitchMeshName = TEXT("Switch");
	BaseMeshName = TEXT("Base");
	LabelName = TEXT("Label");

	// Switch press depth (Z axis)
	PressDepth = 6.0f;  // How far down it goes
	AnimSpeed = 10.0f;

	// Camera
	bDoCameraFocus = false;  // Off by default, enable on one button
	CameraFocusOffset = FVector(-80.0f, 0.0f, 40.0f);
	CameraFocusSpeed = 2.5f;

	bButtonActive = false;
	bIsPressed = false;
	bAnimating = false;
	bWasMouseDown = false;

	SwitchMesh = nullptr;
	BaseMesh = nullptr;
	Label = nullptr;
	DiceCamera = nullptr;

	AnimProgress = 0.0f;

	CameraProgress = 0.0f;
	bCameraFocusing = false;
	bCameraReturning = false;
}

void UIRButtonComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Find meshes
	TArray<UStaticMeshComponent*> Meshes;
	Owner->GetComponents<UStaticMeshComponent>(Meshes);

	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (Mesh->GetName().Contains(SwitchMeshName))
			SwitchMesh = Mesh;
		else if (Mesh->GetName().Contains(BaseMeshName))
			BaseMesh = Mesh;
	}

	// Find label
	TArray<UTextRenderComponent*> Texts;
	Owner->GetComponents<UTextRenderComponent>(Texts);

	for (UTextRenderComponent* Text : Texts)
	{
		if (Text->GetName().Contains(LabelName))
		{
			Label = Text;
			break;
		}
	}

	// If no specific label found, use first one
	if (!Label && Texts.Num() > 0)
	{
		Label = Texts[0];
	}

	// Store initial switch position
	if (SwitchMesh)
	{
		SwitchUnpressedPos = SwitchMesh->GetRelativeLocation();
		SwitchPressedPos = SwitchUnpressedPos;
		SwitchPressedPos.Z -= PressDepth;  // Move down
	}

	// Set label text based on button type
	UpdateLabelText();

	// Find camera
	DiceCamera = FindDiceCamera();
}

void UIRButtonComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update camera animations
	UpdateCameraFocus(DeltaTime);

	// Update switch animation
	if (bAnimating)
	{
		UpdateAnimation(DeltaTime);
		return;
	}

	// Handle click input when active and not already pressed
	if (bButtonActive && !bIsPressed)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			bool bMouseDown = PC->IsInputKeyDown(EKeys::LeftMouseButton);

			// Click detection (bWasMouseDown is now a member variable, not shared between instances)
			if (bMouseDown && !bWasMouseDown && IsMouseOverSwitch())
			{
				OnClicked();
			}

			bWasMouseDown = bMouseDown;
		}
	}
}

void UIRButtonComponent::ActivateButton()
{
	bButtonActive = true;
	bIsPressed = false;
	bAnimating = false;

	// Reset to unpressed position
	if (SwitchMesh)
	{
		ApplySwitchPosition(SwitchUnpressedPos);
	}

	// Update label color to show active
	if (Label)
	{
		if (ButtonType == EIRButtonType::Yes)
		{
			Label->SetTextRenderColor(FColor(100, 255, 100, 255));  // Green
		}
		else
		{
			Label->SetTextRenderColor(FColor(255, 100, 100, 255));  // Red
		}
	}

	// Focus camera (only if enabled)
	if (bDoCameraFocus)
	{
		StartCameraFocus();
	}
}

void UIRButtonComponent::DeactivateButton()
{
	bButtonActive = false;

	// Dim the label
	if (Label)
	{
		Label->SetTextRenderColor(FColor(128, 128, 128, 255));  // Gray
	}

	// Return camera (only if this button controls camera)
	if (bDoCameraFocus)
	{
		StartCameraReturn();
	}
}

void UIRButtonComponent::ResetButton()
{
	bIsPressed = false;
	bAnimating = true;
	SwitchStartPos = SwitchMesh ? SwitchMesh->GetRelativeLocation() : FVector::ZeroVector;
	AnimProgress = 0.0f;
}

void UIRButtonComponent::OnClicked()
{
	if (bIsPressed) return;

	bIsPressed = true;
	bAnimating = true;
	SwitchStartPos = SwitchMesh ? SwitchMesh->GetRelativeLocation() : SwitchUnpressedPos;
	AnimProgress = 0.0f;

	UE_LOG(LogTemp, Warning, TEXT("Button Clicked: %s"), ButtonType == EIRButtonType::Yes ? TEXT("YES") : TEXT("NO"));
}

void UIRButtonComponent::UpdateAnimation(float DeltaTime)
{
	AnimProgress += DeltaTime * AnimSpeed;

	FVector TargetPos = bIsPressed ? SwitchPressedPos : SwitchUnpressedPos;

	if (AnimProgress >= 1.0f)
	{
		AnimProgress = 1.0f;
		bAnimating = false;

		ApplySwitchPosition(TargetPos);

		// Fire event when pressed animation completes
		if (bIsPressed)
		{
			OnButtonPressed.Broadcast(ButtonType);
		}
	}
	else
	{
		// Elastic easing for juicy snap
		float T = EaseOutElastic(AnimProgress);
		FVector NewPos = FMath::Lerp(SwitchStartPos, TargetPos, T);
		ApplySwitchPosition(NewPos);
	}
}

void UIRButtonComponent::ApplySwitchPosition(FVector Position)
{
	if (!SwitchMesh) return;
	SwitchMesh->SetRelativeLocation(Position);
}

void UIRButtonComponent::UpdateLabelText()
{
	if (!Label) return;

	if (ButtonType == EIRButtonType::Yes)
	{
		Label->SetText(FText::FromString(TEXT("YES")));
		Label->SetTextRenderColor(FColor(100, 255, 100, 255));  // Green
	}
	else
	{
		Label->SetText(FText::FromString(TEXT("NO")));
		Label->SetTextRenderColor(FColor(255, 100, 100, 255));  // Red
	}
}

void UIRButtonComponent::StartCameraFocus()
{
	if (!DiceCamera) DiceCamera = FindDiceCamera();
	if (!DiceCamera) return;

	OriginalCameraPos = DiceCamera->GetActorLocation();
	OriginalCameraRot = DiceCamera->GetActorRotation();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		// Position camera to look at button
		FVector ButtonPos = Owner->GetActorLocation();
		TargetCameraPos = ButtonPos + CameraFocusOffset;

		// Look at button
		FVector DirToButton = (ButtonPos - TargetCameraPos).GetSafeNormal();
		TargetCameraRot = DirToButton.Rotation();
	}

	CameraProgress = 0.0f;
	bCameraFocusing = true;
	bCameraReturning = false;
}

void UIRButtonComponent::StartCameraReturn()
{
	CameraProgress = 0.0f;
	bCameraFocusing = false;
	bCameraReturning = true;
}

void UIRButtonComponent::UpdateCameraFocus(float DeltaTime)
{
	if (!DiceCamera) return;

	if (bCameraFocusing)
	{
		CameraProgress += DeltaTime * CameraFocusSpeed;

		if (CameraProgress >= 1.0f)
		{
			CameraProgress = 1.0f;
			bCameraFocusing = false;
			DiceCamera->SetActorLocation(TargetCameraPos);
			DiceCamera->SetActorRotation(TargetCameraRot);
		}
		else
		{
			float T = EaseOutCubic(CameraProgress);
			FVector NewPos = FMath::Lerp(OriginalCameraPos, TargetCameraPos, T);
			FRotator NewRot = FMath::Lerp(OriginalCameraRot, TargetCameraRot, T);
			DiceCamera->SetActorLocation(NewPos);
			DiceCamera->SetActorRotation(NewRot);
		}
	}
	else if (bCameraReturning)
	{
		CameraProgress += DeltaTime * CameraFocusSpeed;

		if (CameraProgress >= 1.0f)
		{
			CameraProgress = 1.0f;
			bCameraReturning = false;
			DiceCamera->SetActorLocation(OriginalCameraPos);
			DiceCamera->SetActorRotation(OriginalCameraRot);
		}
		else
		{
			float T = EaseOutCubic(CameraProgress);
			FVector CurrentPos = DiceCamera->GetActorLocation();
			FRotator CurrentRot = DiceCamera->GetActorRotation();

			FVector NewPos = FMath::Lerp(CurrentPos, OriginalCameraPos, T);
			FRotator NewRot = FMath::Lerp(CurrentRot, OriginalCameraRot, T);
			DiceCamera->SetActorLocation(NewPos);
			DiceCamera->SetActorRotation(NewRot);
		}
	}
}

bool UIRButtonComponent::IsMouseOverSwitch()
{
	if (!SwitchMesh) return false;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return false;

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
		return false;

	FHitResult HitResult;
	FVector TraceEnd = WorldLocation + WorldDirection * 5000.0f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(PC->GetPawn());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, Params))
	{
		if (HitResult.GetComponent() == SwitchMesh)
		{
			return true;
		}
	}

	return false;
}

ADiceCamera* UIRButtonComponent::FindDiceCamera()
{
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADiceCamera::StaticClass(), FoundActors);

	if (FoundActors.Num() > 0)
	{
		return Cast<ADiceCamera>(FoundActors[0]);
	}
	return nullptr;
}

float UIRButtonComponent::EaseOutElastic(float t)
{
	if (t == 0.0f || t == 1.0f) return t;

	float p = 0.3f;
	float s = p / 4.0f;

	return FMath::Pow(2.0f, -10.0f * t) * FMath::Sin((t - s) * (2.0f * PI) / p) + 1.0f;
}

float UIRButtonComponent::EaseOutCubic(float t)
{
	t = t - 1.0f;
	return t * t * t + 1.0f;
}
