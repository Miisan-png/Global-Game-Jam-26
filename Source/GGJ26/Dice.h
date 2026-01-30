#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dice.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class ADice : public AActor
{
	GENERATED_BODY()

public:
	ADice();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<UTextRenderComponent*> FaceTexts;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DiceSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugNumbers;

	UFUNCTION(BlueprintCallable)
	void Throw(FVector Direction, float Force);

	UFUNCTION(BlueprintCallable)
	bool IsStill();

	UFUNCTION(BlueprintCallable)
	int32 GetResult();

	UFUNCTION(BlueprintCallable)
	void SetValue(int32 NewValue);

	UFUNCTION(BlueprintCallable)
	void SetHighlighted(bool bHighlight);

	UFUNCTION(BlueprintCallable)
	void SetMatched(bool bMatch);

	UPROPERTY(BlueprintReadOnly)
	bool bIsHighlighted;

	UPROPERTY(BlueprintReadOnly)
	bool bIsMatched;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentValue;

private:
	bool bHasBeenThrown;
	float HighlightPulse;

	void SetupFaceTexts();
	void DrawFaceNumbers();
	int32 GetFaceValueFromDirection(FVector LocalDirection);
	FVector GetFaceCenter(int32 FaceIndex);
	FVector GetFaceNormal(int32 FaceIndex);
	FRotator GetFaceTextRotation(int32 FaceIndex);
};
