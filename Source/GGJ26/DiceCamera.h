#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DiceCamera.generated.h"

class UCameraComponent;

UCLASS()
class ADiceCamera : public AActor
{
	GENERATED_BODY()

public:
	ADiceCamera();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCameraComponent* Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing")
	float BreathingSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing")
	float BreathingAmplitudeZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing")
	float BreathingAmplitudeRotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Breathing")
	bool bEnableBreathing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bIsMainCamera;

	UFUNCTION(BlueprintCallable)
	void ActivateCamera();

private:
	float TimeAccumulator;
	FVector InitialPosition;
	FRotator InitialRotation;
};
