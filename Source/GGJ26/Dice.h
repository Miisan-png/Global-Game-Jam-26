#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dice.generated.h"

class UStaticMeshComponent;

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

private:
	bool bHasBeenThrown;

	void DrawFaceNumbers();
	int32 GetFaceValueFromDirection(FVector LocalDirection);
	FVector GetFaceCenter(int32 FaceIndex);
	FVector GetFaceNormal(int32 FaceIndex);
};
