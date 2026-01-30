#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dice.h"
#include "DiceModifier.h"
#include "DiceGameManager.generated.h"

class AMaskEnemy;
class ADiceCamera;

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	Idle,
	EnemyThrowing,
	EnemyDiceSettling,
	EnemyDiceLining,
	PlayerTurn,
	PlayerThrowing,
	PlayerDiceSettling,
	PlayerDiceLining,
	PlayerMatching,
	RoundEnd,
	GameOver
};

UCLASS()
class ADiceGameManager : public AActor
{
	GENERATED_BODY()

public:
	ADiceGameManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	int32 EnemyNumDice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	int32 PlayerNumDice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	float DiceThrowForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FVector EnemyDiceSpawnOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	AActor* TableActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	float DiceLineupSpacing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	float EnemyDiceOffsetY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	float PlayerDiceOffsetY;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	float DiceHeightAboveTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	float DiceLineupSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugGizmos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	int32 PlayerHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	int32 EnemyHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	int32 MaxHealth;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	EGamePhase CurrentPhase;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<ADice*> EnemyDice;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<ADice*> PlayerDice;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<int32> EnemyResults;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<int32> PlayerResults;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	ADice* SelectedPlayerDice;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 SelectedDiceIndex;

	UPROPERTY(BlueprintReadOnly, Category = "Modifiers")
	TArray<ADiceModifier*> AllModifiers;

	UFUNCTION(BlueprintCallable)
	void StartGame();

	UFUNCTION(BlueprintCallable)
	void PlayerThrowDice();

private:
	void SetupInputBindings();
	void OnStartGamePressed();
	void OnPlayerThrowPressed();
	void OnToggleDebugPressed();
	void OnSelectNext();
	void OnSelectPrev();
	void OnConfirmSelection();
	void OnCancelSelection();
	void UpdateDiceDebugVisibility();

	void EnemyThrowDice();
	void CheckEnemyDiceSettled(float DeltaTime);
	void PrepareEnemyDiceLineup();
	void LineUpEnemyDice(float DeltaTime);
	void StartPlayerTurn();
	void CheckPlayerDiceSettled();
	void PreparePlayerDiceLineup();
	void LineUpPlayerDice(float DeltaTime);

	void StartMatchingPhase();
	void UpdateMatchingPhase();
	void SelectDice(int32 Index);
	void TryMatchDice(int32 PlayerIndex, int32 EnemyIndex);
	void TryApplyModifier(ADiceModifier* Modifier);
	void UpdateSelectionHighlights();
	void CheckAllMatched();
	void DealDamage(bool bToEnemy);
	void CheckGameOver();

	void FindAllModifiers();
	void ClearAllDice();
	void DrawTurnText();
	void DrawHealthBars();

	FRotator GetRotationForFaceUp(int32 FaceValue);
	float SmoothStep(float t);
	float EaseOutCubic(float t);

	AMaskEnemy* FindEnemy();
	ADiceCamera* FindCamera();

	bool bEnemyDiceSettled;
	bool bPlayerDiceSettled;
	float LineupProgress;
	float PlayerLineupProgress;
	float WaitTimer;
	float StaggerDelay;

	TArray<FVector> EnemyDiceStartPositions;
	TArray<FRotator> EnemyDiceStartRotations;
	TArray<FVector> EnemyDiceTargetPositions;
	TArray<FRotator> EnemyDiceTargetRotations;

	TArray<FVector> PlayerDiceStartPositions;
	TArray<FRotator> PlayerDiceStartRotations;
	TArray<FVector> PlayerDiceTargetPositions;
	TArray<FRotator> PlayerDiceTargetRotations;

	TArray<bool> PlayerDiceMatched;
	TArray<bool> EnemyDiceMatched;
	int32 SelectionMode;
	int32 HoveredEnemyIndex;
	int32 HoveredModifierIndex;
};
