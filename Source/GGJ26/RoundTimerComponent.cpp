#include "RoundTimerComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

URoundTimerComponent::URoundTimerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Defaults
	RoundTime = 30.0f;
	TimerLabelName = TEXT("TimerLabel");

	NormalColor = FColor::White;
	TimerColor = FColor::Red;

	CurrentState = ETimerState::Idle;
	TimeRemaining = RoundTime;
	bIsRunning = false;

	TimerLabel = nullptr;

	// Text reveal animation
	bTextRevealing = false;
	TextRevealProgress = 0.0f;
	TextRevealSpeed = 15.0f;  // Characters per second
	TargetText = TEXT("");
	RevealedCharCount = 0;

	// Idle cycle
	IdleCycleTime = 2.0f;  // Switch every 2 seconds
	IdleCycleTimer = 0.0f;
	IdleCycleIndex = 0;
	LastIdleText = TEXT("");
}

void URoundTimerComponent::BeginPlay()
{
	Super::BeginPlay();

	// Find the timer label text component
	AActor* Owner = GetOwner();
	if (Owner)
	{
		TArray<UTextRenderComponent*> TextComponents;
		Owner->GetComponents<UTextRenderComponent>(TextComponents);

		for (UTextRenderComponent* Text : TextComponents)
		{
			if (Text->GetName().Contains(TimerLabelName))
			{
				TimerLabel = Text;
				break;
			}
		}

		if (!TimerLabel && TextComponents.Num() > 0)
		{
			// Use first text component if specific one not found
			TimerLabel = TextComponents[0];
		}
	}

	// Start in idle state
	SetIdle();
}

void URoundTimerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Handle countdown
	if (bIsRunning && CurrentState == ETimerState::Countdown)
	{
		TimeRemaining -= DeltaTime;

		if (TimeRemaining <= 0.0f)
		{
			TimeRemaining = 0.0f;
			bIsRunning = false;
			CurrentState = ETimerState::TimeUp;
			OnTimerExpired.Broadcast();
		}
	}

	// Handle idle cycling
	if (CurrentState == ETimerState::Idle && !bTextRevealing)
	{
		IdleCycleTimer += DeltaTime;
		if (IdleCycleTimer >= IdleCycleTime)
		{
			IdleCycleTimer = 0.0f;
			IdleCycleIndex = (IdleCycleIndex + 1) % 2;

			FString NewText = (IdleCycleIndex == 0) ? TEXT("WELCOME") : TEXT("Miss Ada");
			if (NewText != LastIdleText)
			{
				LastIdleText = NewText;
				StartTextReveal(NewText);
			}
		}
	}

	// Handle text reveal animation
	if (bTextRevealing)
	{
		UpdateTextReveal(DeltaTime);
	}

	// Update display
	UpdateDisplay();
}

void URoundTimerComponent::SetState(ETimerState NewState)
{
	CurrentState = NewState;

	switch (NewState)
	{
		case ETimerState::Idle:
			bIsRunning = false;
			IdleCycleTimer = 0.0f;
			IdleCycleIndex = 0;
			break;
		case ETimerState::Hold:
			bIsRunning = false;
			break;
		case ETimerState::Dealing:
			bIsRunning = false;
			break;
		case ETimerState::PlayerReady:
			bIsRunning = false;
			break;
		case ETimerState::Countdown:
			// Don't auto-start, use StartCountdown()
			break;
		case ETimerState::TimeUp:
			bIsRunning = false;
			break;
	}
}

void URoundTimerComponent::StartCountdown()
{
	CurrentState = ETimerState::Countdown;
	TimeRemaining = RoundTime;
	bIsRunning = true;

	// Start reveal animation for initial time display
	StartTextReveal(FormatTime(TimeRemaining));
}

void URoundTimerComponent::StopCountdown()
{
	bIsRunning = false;
}

void URoundTimerComponent::ResetTimer()
{
	TimeRemaining = RoundTime;
	bIsRunning = false;
	CurrentState = ETimerState::Idle;
}

void URoundTimerComponent::SetIdle()
{
	SetState(ETimerState::Idle);
	LastIdleText = TEXT("WELCOME");
	StartTextReveal(TEXT("WELCOME"));
}

void URoundTimerComponent::SetHold()
{
	SetState(ETimerState::Hold);
	StartTextReveal(TEXT("HOLD"));
}

void URoundTimerComponent::SetDealing()
{
	SetState(ETimerState::Dealing);
	StartTextReveal(TEXT("DEALING"));
}

void URoundTimerComponent::SetPlayerReady()
{
	SetState(ETimerState::PlayerReady);
	StartTextReveal(TEXT("E TO BLITZ"));
}

void URoundTimerComponent::StartTextReveal(const FString& NewText)
{
	TargetText = NewText;
	bTextRevealing = true;
	TextRevealProgress = 0.0f;
	RevealedCharCount = 0;
}

void URoundTimerComponent::UpdateTextReveal(float DeltaTime)
{
	TextRevealProgress += DeltaTime * TextRevealSpeed;

	int32 NewCharCount = FMath::FloorToInt(TextRevealProgress);

	if (NewCharCount >= TargetText.Len())
	{
		// Reveal complete
		bTextRevealing = false;
		RevealedCharCount = TargetText.Len();
	}
	else
	{
		RevealedCharCount = NewCharCount;
	}
}

void URoundTimerComponent::UpdateDisplay()
{
	if (!TimerLabel) return;

	FString DisplayText;
	FColor DisplayColor = NormalColor;

	switch (CurrentState)
	{
		case ETimerState::Idle:
		{
			DisplayColor = NormalColor;

			FString IdleText = (IdleCycleIndex == 0) ? TEXT("WELCOME") : TEXT("Miss Ada");
			if (bTextRevealing)
			{
				DisplayText = GetRevealedText(IdleText);
			}
			else
			{
				DisplayText = IdleText;
			}
			break;
		}

		case ETimerState::Hold:
		{
			DisplayColor = NormalColor;

			if (bTextRevealing)
			{
				DisplayText = GetRevealedText(TEXT("HOLD"));
			}
			else
			{
				DisplayText = TEXT("HOLD");
			}
			break;
		}

		case ETimerState::Dealing:
		{
			DisplayColor = NormalColor;

			if (bTextRevealing)
			{
				DisplayText = GetRevealedText(TEXT("DEALING"));
			}
			else
			{
				DisplayText = TEXT("DEALING");
			}
			break;
		}

		case ETimerState::PlayerReady:
		{
			DisplayColor = NormalColor;

			if (bTextRevealing)
			{
				DisplayText = GetRevealedText(TEXT("E TO BLITZ"));
			}
			else
			{
				DisplayText = TEXT("E TO BLITZ");
			}
			break;
		}

		case ETimerState::Countdown:
		{
			DisplayText = FormatTime(TimeRemaining);
			DisplayColor = TimerColor;
			break;
		}

		case ETimerState::TimeUp:
		{
			DisplayText = TEXT("00:00");
			DisplayColor = TimerColor;
			break;
		}
	}

	TimerLabel->SetText(FText::FromString(DisplayText));
	TimerLabel->SetTextRenderColor(DisplayColor);
}

FString URoundTimerComponent::FormatTime(float Seconds)
{
	int32 TotalSeconds = FMath::FloorToInt(Seconds);
	int32 Secs = TotalSeconds % 60;
	int32 Hundredths = FMath::FloorToInt((Seconds - TotalSeconds) * 100);

	return FString::Printf(TEXT("%02d:%02d"), Secs, Hundredths);
}

FString URoundTimerComponent::GetRevealedText(const FString& FullText)
{
	FString Result;

	// Random characters for the "scramble" effect
	TArray<TCHAR> ScrambleChars = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	                                 'A', 'B', 'C', 'D', 'E', 'F', 'X', '#', '@', '%' };

	for (int32 i = 0; i < FullText.Len(); i++)
	{
		if (i < RevealedCharCount)
		{
			// This character is revealed
			Result.AppendChar(FullText[i]);
		}
		else
		{
			// This character is still scrambled - pick random char
			int32 RandomIndex = FMath::RandRange(0, ScrambleChars.Num() - 1);
			Result.AppendChar(ScrambleChars[RandomIndex]);
		}
	}

	return Result;
}
