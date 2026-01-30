// Global Game Jam 2026 - Dice Game Mode

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameModeDice.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDiceRolled, int32, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameEnded, bool, bWon);

UCLASS(config=Game)
class AGameModeDice : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGameModeDice();

	virtual void BeginPlay() override;

	// Roll a dice with specified number of sides (default 6)
	UFUNCTION(BlueprintCallable, Category = "Dice")
	int32 RollDice(int32 Sides = 6);

	// Roll multiple dice and return the sum
	UFUNCTION(BlueprintCallable, Category = "Dice")
	int32 RollMultipleDice(int32 NumDice, int32 Sides = 6);

	// Start the game
	UFUNCTION(BlueprintCallable, Category = "Game")
	virtual void StartGame();

	// End the game
	UFUNCTION(BlueprintCallable, Category = "Game")
	virtual void EndGame(bool bWon);

	// Reset the game state
	UFUNCTION(BlueprintCallable, Category = "Game")
	virtual void ResetGame();

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Dice|Events")
	FOnDiceRolled OnDiceRolled;

	UPROPERTY(BlueprintAssignable, Category = "Game|Events")
	FOnGameStarted OnGameStarted;

	UPROPERTY(BlueprintAssignable, Category = "Game|Events")
	FOnGameEnded OnGameEnded;

	// Game state
	UPROPERTY(BlueprintReadOnly, Category = "Game")
	bool bGameInProgress;

	UPROPERTY(BlueprintReadOnly, Category = "Game")
	int32 CurrentRound;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Game")
	int32 MaxRounds;

	// Last dice roll result
	UPROPERTY(BlueprintReadOnly, Category = "Dice")
	int32 LastRollResult;
};
