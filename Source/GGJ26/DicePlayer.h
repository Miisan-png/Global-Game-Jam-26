#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Dice.h"
#include "DicePlayer.generated.h"

class ADiceCamera;

UCLASS(config=Game)
class ADicePlayer : public APawn
{
	GENERATED_BODY()

public:
	ADicePlayer();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent* Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* PlaceholderMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	bool bShowPlaceholderInGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice")
	int32 NumDiceToThrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice")
	float ThrowForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice")
	TSubclassOf<ADice> DiceClass;

	UPROPERTY(BlueprintReadOnly, Category = "Dice")
	TArray<ADice*> SpawnedDice;

	UFUNCTION(BlueprintCallable)
	void ThrowDice();

	UFUNCTION(BlueprintCallable)
	void ClearDice();

	UFUNCTION(BlueprintCallable)
	bool AreAllDiceStill();

	UFUNCTION(BlueprintCallable)
	TArray<int32> GetDiceResults();

private:
	void OnThrowPressed();
	ADiceCamera* FindCamera();
};
