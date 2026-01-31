#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "PlayerHandComponent.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class ADiceCamera;
class ASoundManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChopComplete);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UPlayerHandComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerHandComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Mesh names to search for (set these to match your blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString PalmMeshName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Finger1Name;  // Thumb

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Finger2Name;  // Index

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Finger3Name;  // Middle

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Finger4Name;  // Ring

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Finger5Name;  // Pinky

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Knife1Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Knife2Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Knife3Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Knife4Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Names")
	FString Knife5Name;

	// Found mesh references (auto-populated in BeginPlay)
	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* PalmMesh;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Finger1;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Finger2;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Finger3;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Finger4;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Finger5;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Knife1;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Knife2;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Knife3;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Knife4;

	UPROPERTY(BlueprintReadOnly, Category = "Hand")
	UStaticMeshComponent* Knife5;

	// Animation settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	FVector KnifeStartOffset;  // How far up the knife starts

	// Per-finger knife positions (Down position, Slice position)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife5DownPos;  // Pinky
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife5SlicePos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife4DownPos;  // Ring
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife4SlicePos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife3DownPos;  // Middle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife3SlicePos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife2DownPos;  // Index
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife2SlicePos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife1DownPos;  // Thumb
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Positions")
	FVector Knife1SlicePos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float KnifeDownSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float KnifeSliceSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float WaitTimeBeforeSlice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float FingerImpulseStrength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float KnifeReturnSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float CamShakeIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float CamShakeDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float KnifeSliceRotation;  // How much knife rotates during slice

	// Hand Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hand Settings")
	bool bIsPlayerHand;  // True = player, False = enemy

	// Sound
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	ASoundManager* SoundManager;

	// Blood VFX
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood VFX")
	UNiagaraSystem* BloodParticleSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood VFX")
	FLinearColor BloodColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood VFX")
	int32 BloodSplatterCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blood VFX")
	float BloodSplatterSpread;

	// Camera zoom on chop (for enemy hand dramatic effect)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float ChopCameraZoomAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	float ChopCameraZoomSpeed;

	// Current state
	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 CurrentFingerToChop;  // 5 = pinky, 1 = thumb, 0 = all gone

	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 FingersRemaining;

	UFUNCTION(BlueprintCallable)
	void ChopNextFinger();

	UFUNCTION(BlueprintCallable)
	void TriggerChop();  // Called by input

	// Called when chop animation is fully complete (knife returned)
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnChopComplete OnChopComplete;

private:
	void SetupInputBindings();

	// Animation state
	bool bIsAnimating;
	int32 AnimationPhase;  // 0=idle, 1=knife coming down, 2=waiting, 3=slicing, 4=returning up, 5=done
	float AnimationTimer;
	FVector KnifeAnimStartPos;
	FVector KnifeAnimTargetPos;
	FVector KnifeReturnPos;  // Where knife returns to
	FRotator KnifeStartRot;
	FRotator KnifeTargetRot;
	UStaticMeshComponent* CurrentKnife;
	UStaticMeshComponent* CurrentFinger;

	FVector GetKnifeDownPos(int32 FingerIndex);
	FVector GetKnifeSlicePos(int32 FingerIndex);

	void UpdateAnimation(float DeltaTime);
	void StartKnifeDown();
	void StartSlice();
	void StartKnifeReturn();
	void FinishChop();
	void PlayCameraShake();
	void UpdateCameraShake(float DeltaTime);
	void SpawnBloodVFX();

	// Camera shake state
	bool bCameraShaking;
	float CameraShakeTimer;
	FVector ShakeOffset;  // Current shake offset to apply

	// Camera zoom state
	bool bCameraZooming;
	float CameraZoomProgress;
	FVector CameraZoomStartPos;
	FVector CameraZoomTargetPos;
	FVector CameraZoomOriginalPos;  // Original position before any zoom
	FRotator CameraZoomStartRot;
	FRotator CameraZoomTargetRot;
	FRotator CameraZoomOriginalRot;
	bool bCameraZoomingOut;

	void StartCameraZoomIn();
	void StartCameraZoomOut();
	void UpdateCameraZoom(float DeltaTime);
	ADiceCamera* FindDiceCamera();
	ADiceCamera* CachedDiceCamera;

	UStaticMeshComponent* GetFingerMesh(int32 FingerIndex);
	UStaticMeshComponent* GetKnifeMesh(int32 FingerIndex);
};
