#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dice.h"
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
	RoundEnd
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

	UFUNCTION(BlueprintCallable)
	void StartGame();

	UFUNCTION(BlueprintCallable)
	void PlayerThrowDice();

private:
	void SetupInputBindings();
	void OnStartGamePressed();
	void OnPlayerThrowPressed();
	void OnToggleDebugPressed();
	void UpdateDiceDebugVisibility();

	void EnemyThrowDice();
	void CheckEnemyDiceSettled(float DeltaTime);
	void LineUpEnemyDice(float DeltaTime);
	void StartPlayerTurn();
	void CheckPlayerDiceSettled();
	void PreparePlayerDiceLineup();
	void LineUpPlayerDice(float DeltaTime);

	void ClearAllDice();
	void DrawTurnText();
	void PrepareEnemyDiceLineup();
	FRotator GetRotationForFaceUp(int32 FaceValue);
	float SmoothStep(float t);
	float EaseOutCubic(float t);

	AMaskEnemy* FindEnemy();
	ADiceCamera* FindCamera();

	bool bEnemyDiceSettled;
	bool bPlayerDiceSettled;
	float LineupProgress;
	float WaitTimer;
	float StaggerDelay;

	TArray<FVector> EnemyDiceStartPositions;
	TArray<FRotator> EnemyDiceStartRotations;
	TArray<FVector> EnemyDiceTargetPositions;
	TArray<FRotator> EnemyDiceTargetRotations;

	float PlayerLineupProgress;
	TArray<FVector> PlayerDiceStartPositions;
	TArray<FRotator> PlayerDiceStartRotations;
	TArray<FVector> PlayerDiceTargetPositions;
	TArray<FRotator> PlayerDiceTargetRotations;
};
