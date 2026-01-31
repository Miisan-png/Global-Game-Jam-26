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
	RerollAll,
	BonusHigher,   // Bonus round: guess higher than 7
	BonusLower     // Bonus round: guess lower than 7
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

	UPROPERTY(BlueprintReadOnly, Category = "Modifier")
	bool bIsInvalid;  // Can't be used with current dice value

	UPROPERTY(BlueprintReadOnly, Category = "Modifier")
	bool bIsActive;

	UPROPERTY(BlueprintReadOnly, Category = "Modifier")
	bool bIsHidden;

	UFUNCTION(BlueprintCallable)
	void SetActive(bool bActive);

	UFUNCTION(BlueprintCallable)
	void SetHidden(bool bHide);

	UFUNCTION(BlueprintCallable)
	void SetHighlighted(bool bHighlight);

	UFUNCTION(BlueprintCallable)
	void SetInvalid(bool bInvalid);

	UFUNCTION(BlueprintCallable)
	bool CanApplyToValue(int32 Value);

	UFUNCTION(BlueprintCallable)
	void UseModifier();

	UFUNCTION(BlueprintCallable)
	int32 ApplyToValue(int32 OriginalValue);

	UFUNCTION(BlueprintCallable)
	FString GetModifierDisplayText();

	UFUNCTION(BlueprintCallable)
	bool RequiresDiceSelection();

	UFUNCTION(BlueprintCallable)
	void UpdateBasePosition();

private:
	void UpdateVisuals();
	FLinearColor BaseColor;
	FLinearColor HighlightColor;
	FLinearColor UsedColor;
	FLinearColor InactiveColor;

	float ActivationProgress;
	bool bActivating;

	// Hover float animation
	FVector BasePosition;
	bool bBasePositionSet;
	float HoverProgress;
	float HoverPulse;
};
