#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SoundManager.generated.h"

class USoundBase;

UCLASS()
class ASoundManager : public AActor
{
	GENERATED_BODY()

public:
	ASoundManager();

	// Sound assets - drag your SFX here
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sounds")
	USoundBase* DiceRollSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sounds")
	USoundBase* DiceMatchSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sounds")
	USoundBase* KnifeCutSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sounds")
	USoundBase* PickUpSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sounds")
	USoundBase* HoverSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sounds")
	USoundBase* ErrorSound;

	// Volume settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MasterVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SFXVolume;

	// Pitch settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float HoverPitch;  // Higher = faster/snappier

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ClampMin = "0.8", ClampMax = "1.2"))
	float DiceRollPitchVariation;  // Random pitch variation for dice

	// Play functions
	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayDiceRoll();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayDiceMatch();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayKnifeCut();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayPickUp();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayHover();

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayError();

	// Play at location (for 3D sounds) - used for individual dice
	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayDiceRollAtLocation(FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Sound")
	void PlayDiceMatchAtLocation(FVector Location);

private:
	void PlaySound2D(USoundBase* Sound, float Pitch = 1.0f);
	void PlaySoundAtLocation(USoundBase* Sound, FVector Location, float Pitch = 1.0f);
};
