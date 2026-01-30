// Global Game Jam 2026 - Dice Game Mode

#include "GameModeDice.h"

AGameModeDice::AGameModeDice()
{
	bGameInProgress = false;
	CurrentRound = 0;
	MaxRounds = 10;
	LastRollResult = 0;
}

void AGameModeDice::BeginPlay()
{
	Super::BeginPlay();
}

int32 AGameModeDice::RollDice(int32 Sides)
{
	if (Sides < 2)
	{
		Sides = 2;
	}

	LastRollResult = FMath::RandRange(1, Sides);
	OnDiceRolled.Broadcast(LastRollResult);

	return LastRollResult;
}

int32 AGameModeDice::RollMultipleDice(int32 NumDice, int32 Sides)
{
	int32 Total = 0;

	for (int32 i = 0; i < NumDice; i++)
	{
		Total += RollDice(Sides);
	}

	return Total;
}

void AGameModeDice::StartGame()
{
	bGameInProgress = true;
	CurrentRound = 1;
	OnGameStarted.Broadcast();
}

void AGameModeDice::EndGame(bool bWon)
{
	bGameInProgress = false;
	OnGameEnded.Broadcast(bWon);
}

void AGameModeDice::ResetGame()
{
	bGameInProgress = false;
	CurrentRound = 0;
	LastRollResult = 0;
}
