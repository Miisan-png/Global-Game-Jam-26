#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DiceModifier.generated.h"

class UBoxComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EModifierType : uint8
{
	None,
	MinusOne,
	PlusOne,
	PlusTwo,
	Flip,
	RerollOne,
	RerollAll
};

UCLASS()
class ADiceModifier : public AActor
{
	GENERATED_BODY()

public:
	ADiceModifier();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* PlaneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UTextRenderComponent* ModifierText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modifier")
	EModifierType ModifierType;

	UPROPERTY(BlueprintReadOnly, Category = "Modifier")
	bool bIsUsed;

	UPROPERTY(BlueprintReadOnly, Category = "Modifier")
	bool bIsHighlighted;

	UFUNCTION(BlueprintCallable)
	void SetHighlighted(bool bHighlight);

	UFUNCTION(BlueprintCallable)
	void UseModifier();

	UFUNCTION(BlueprintCallable)
	int32 ApplyToValue(int32 OriginalValue);

	UFUNCTION(BlueprintCallable)
	FString GetModifierDisplayText();

	UFUNCTION(BlueprintCallable)
	bool RequiresDiceSelection();

private:
	void UpdateVisuals();
	FLinearColor BaseColor;
	FLinearColor HighlightColor;
	FLinearColor UsedColor;
};
