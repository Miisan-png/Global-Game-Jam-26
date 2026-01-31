#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IRButtonComponent.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class ADiceCamera;

UENUM(BlueprintType)
enum class EIRButtonType : uint8
{
	Yes,
	No
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnButtonPressed, EIRButtonType, ButtonType);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UIRButtonComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIRButtonComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Button type - set in inspector
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button")
	EIRButtonType ButtonType;

	// Mesh names
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString SwitchMeshName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString BaseMeshName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString LabelName;

	// Switch press settings (Z axis movement)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Switch")
	float PressDepth;  // How far down the switch goes when pressed

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Switch")
	float AnimSpeed;

	// Camera focus settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	bool bDoCameraFocus;  // Enable camera focus for this button

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector CameraFocusOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraFocusSpeed;

	// State
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bButtonActive;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsPressed;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnButtonPressed OnButtonPressed;

	// Functions
	UFUNCTION(BlueprintCallable)
	void ActivateButton();

	UFUNCTION(BlueprintCallable)
	void DeactivateButton();

	UFUNCTION(BlueprintCallable)
	void ResetButton();

private:
	UStaticMeshComponent* SwitchMesh;
	UStaticMeshComponent* BaseMesh;
	UTextRenderComponent* Label;

	// Switch animation
	bool bAnimating;
	FVector SwitchStartPos;
	FVector SwitchUnpressedPos;
	FVector SwitchPressedPos;
	float AnimProgress;

	// Camera
	ADiceCamera* DiceCamera;
	FVector OriginalCameraPos;
	FRotator OriginalCameraRot;
	FVector TargetCameraPos;
	FRotator TargetCameraRot;
	float CameraProgress;
	bool bCameraFocusing;
	bool bCameraReturning;

	void OnClicked();
	void UpdateAnimation(float DeltaTime);
	void UpdateCameraFocus(float DeltaTime);
	void ApplySwitchPosition(FVector Position);
	void UpdateLabelText();
	bool IsMouseOverSwitch();

	void StartCameraFocus();
	void StartCameraReturn();
	ADiceCamera* FindDiceCamera();

	float EaseOutElastic(float t);
	float EaseOutCubic(float t);
};
