#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RoundTimerComponent.generated.h"

class UTextRenderComponent;

UENUM(BlueprintType)
enum class ETimerState : uint8
{
	Idle,       // Cycles WELCOME / Miss Ada
	Dealing,    // Enemy throwing dice
	Hold,       // Waiting
	PlayerReady, // E TO BLITZ
	Countdown,  // Timer running
	TimeUp      // 00:00
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTimerExpired);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class URoundTimerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URoundTimerComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	float RoundTime;  // Time in seconds (default 30)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	FString TimerLabelName;  // Name of the text component to find

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	FColor NormalColor;  // Color for text (white)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	FColor TimerColor;  // Color for countdown timer (red)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	float TextRevealSpeed;  // Characters per second for reveal effect

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer")
	float IdleCycleTime;  // Time between WELCOME and Miss Ada

	// State
	UPROPERTY(BlueprintReadOnly, Category = "State")
	ETimerState CurrentState;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	float TimeRemaining;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsRunning;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnTimerExpired OnTimerExpired;

	// Functions
	UFUNCTION(BlueprintCallable)
	void SetState(ETimerState NewState);

	UFUNCTION(BlueprintCallable)
	void StartCountdown();

	UFUNCTION(BlueprintCallable)
	void StopCountdown();

	UFUNCTION(BlueprintCallable)
	void ResetTimer();

	UFUNCTION(BlueprintCallable)
	void SetIdle();

	UFUNCTION(BlueprintCallable)
	void SetHold();

	UFUNCTION(BlueprintCallable)
	void SetDealing();

	UFUNCTION(BlueprintCallable)
	void SetPlayerReady();  // E TO BLITZ

private:
	UTextRenderComponent* TimerLabel;

	void UpdateDisplay();
	FString FormatTime(float Seconds);

	// Text reveal animation
	void StartTextReveal(const FString& NewText);
	void UpdateTextReveal(float DeltaTime);
	FString GetRevealedText(const FString& FullText);

	bool bTextRevealing;
	float TextRevealProgress;
	FString TargetText;
	int32 RevealedCharCount;

	// Idle cycle
	float IdleCycleTimer;
	int32 IdleCycleIndex;  // 0 = WELCOME, 1 = Miss Ada
	FString LastIdleText;
};
