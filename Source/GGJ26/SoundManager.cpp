#include "SoundManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ASoundManager::ASoundManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// Sound assets
	DiceRollSound = nullptr;
	DiceMatchSound = nullptr;
	KnifeCutSound = nullptr;
	PickUpSound = nullptr;
	HoverSound = nullptr;
	ErrorSound = nullptr;

	// Default volumes
	MasterVolume = 1.0f;
	SFXVolume = 1.0f;

	// Pitch settings
	HoverPitch = 1.3f;  // Slightly faster/snappier
	DiceRollPitchVariation = 0.15f;  // +/- 15% random pitch
}

void ASoundManager::PlayDiceRoll()
{
	// Random pitch variation for variety
	float RandomPitch = 1.0f + FMath::RandRange(-DiceRollPitchVariation, DiceRollPitchVariation);
	PlaySound2D(DiceRollSound, RandomPitch);
}

void ASoundManager::PlayDiceMatch()
{
	PlaySound2D(DiceMatchSound);
}

void ASoundManager::PlayKnifeCut()
{
	PlaySound2D(KnifeCutSound);
}

void ASoundManager::PlayPickUp()
{
	PlaySound2D(PickUpSound);
}

void ASoundManager::PlayHover()
{
	PlaySound2D(HoverSound, HoverPitch);
}

void ASoundManager::PlayError()
{
	PlaySound2D(ErrorSound);
}

void ASoundManager::PlayDiceRollAtLocation(FVector Location)
{
	// Random pitch variation for variety
	float RandomPitch = 1.0f + FMath::RandRange(-DiceRollPitchVariation, DiceRollPitchVariation);
	PlaySoundAtLocation(DiceRollSound, Location, RandomPitch);
}

void ASoundManager::PlayDiceMatchAtLocation(FVector Location)
{
	PlaySoundAtLocation(DiceMatchSound, Location);
}

void ASoundManager::PlaySound2D(USoundBase* Sound, float Pitch)
{
	if (Sound)
	{
		float FinalVolume = MasterVolume * SFXVolume;
		UGameplayStatics::PlaySound2D(this, Sound, FinalVolume, Pitch);
	}
}

void ASoundManager::PlaySoundAtLocation(USoundBase* Sound, FVector Location, float Pitch)
{
	if (Sound)
	{
		float FinalVolume = MasterVolume * SFXVolume;
		UGameplayStatics::PlaySoundAtLocation(this, Sound, Location, FRotator::ZeroRotator, FinalVolume, Pitch);
	}
}
