#include "DiceGameManager.h"
#include "MaskEnemy.h"
#include "DiceCamera.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "Camera/CameraComponent.h"

ADiceGameManager::ADiceGameManager()
{
	PrimaryActorTick.bCanEverTick = true;

	EnemyNumDice = 4;
	PlayerNumDice = 5;
	DiceThrowForce = 400.0f;
	EnemyDiceSpawnOffset = FVector(0.0f, 0.0f, 80.0f);
	DiceLineupSpacing = 20.0f;
	DiceLineupSpeed = 2.0f;
	EnemyDiceOffsetY = 0.0f;
	PlayerDiceOffsetY = 0.0f;
	DiceHeightAboveTable = 0.0f;
	StaggerDelay = 0.12f;

	TableActor = nullptr;

	MaxHealth = 5;
	PlayerHealth = 5;
	EnemyHealth = 5;

	CurrentPhase = EGamePhase::Idle;
	bEnemyDiceSettled = false;
	bPlayerDiceSettled = false;
	LineupProgress = 0.0f;
	PlayerLineupProgress = 0.0f;
	WaitTimer = 0.0f;
	bShowDebugGizmos = true;

	SelectedPlayerDice = nullptr;
	SelectedDiceIndex = -1;
	SelectionMode = 0;
	HoveredEnemyIndex = -1;
	HoveredModifierIndex = -1;
}

void ADiceGameManager::BeginPlay()
{
	Super::BeginPlay();
	SetupInputBindings();
	FindAllModifiers();
}

void ADiceGameManager::SetupInputBindings()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		EnableInput(PC);
		if (InputComponent)
		{
			InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ADiceGameManager::OnStartGamePressed);
			InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ADiceGameManager::OnPlayerThrowPressed);
			InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ADiceGameManager::OnToggleDebugPressed);
			InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ADiceGameManager::OnSelectPrev);
			InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ADiceGameManager::OnSelectNext);
			InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &ADiceGameManager::OnSelectPrev);
			InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &ADiceGameManager::OnSelectNext);
			InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ADiceGameManager::OnConfirmSelection);
			InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ADiceGameManager::OnConfirmSelection);
			InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ADiceGameManager::OnCancelSelection);
			InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ADiceGameManager::OnCancelSelection);
		}
	}
}

void ADiceGameManager::FindAllModifiers()
{
	AllModifiers.Empty();
	TArray<AActor*> FoundModifiers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADiceModifier::StaticClass(), FoundModifiers);
	for (AActor* A : FoundModifiers)
	{
		ADiceModifier* Mod = Cast<ADiceModifier>(A);
		if (Mod)
		{
			AllModifiers.Add(Mod);
		}
	}
}

void ADiceGameManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DrawTurnText();
	DrawHealthBars();

	switch (CurrentPhase)
	{
		case EGamePhase::EnemyDiceSettling:
			CheckEnemyDiceSettled(DeltaTime);
			break;

		case EGamePhase::EnemyDiceLining:
			LineUpEnemyDice(DeltaTime);
			break;

		case EGamePhase::PlayerDiceSettling:
			CheckPlayerDiceSettled();
			break;

		case EGamePhase::PlayerDiceLining:
			LineUpPlayerDice(DeltaTime);
			break;

		case EGamePhase::PlayerMatching:
			UpdateMatchingPhase();
			break;

		default:
			break;
	}
}

void ADiceGameManager::OnStartGamePressed()
{
	if (CurrentPhase == EGamePhase::Idle || CurrentPhase == EGamePhase::RoundEnd)
	{
		StartGame();
	}
	else if (CurrentPhase == EGamePhase::GameOver)
	{
		PlayerHealth = MaxHealth;
		EnemyHealth = MaxHealth;
		for (ADiceModifier* Mod : AllModifiers)
		{
			if (Mod)
			{
				Mod->bIsUsed = false;
			}
		}
		StartGame();
	}
}

void ADiceGameManager::OnPlayerThrowPressed()
{
	if (CurrentPhase == EGamePhase::PlayerTurn)
	{
		PlayerThrowDice();
	}
}

void ADiceGameManager::OnToggleDebugPressed()
{
	bShowDebugGizmos = !bShowDebugGizmos;
	UpdateDiceDebugVisibility();
}

void ADiceGameManager::OnSelectNext()
{
	if (CurrentPhase != EGamePhase::PlayerMatching) return;

	if (SelectionMode == 0)
	{
		int32 StartIndex = SelectedDiceIndex;
		do {
			SelectedDiceIndex = (SelectedDiceIndex + 1) % PlayerDice.Num();
		} while (PlayerDiceMatched.IsValidIndex(SelectedDiceIndex) &&
				 PlayerDiceMatched[SelectedDiceIndex] &&
				 SelectedDiceIndex != StartIndex);

		SelectDice(SelectedDiceIndex);
	}
	else if (SelectionMode == 1)
	{
		int32 TotalOptions = EnemyDice.Num() + AllModifiers.Num();
		int32 CurrentOption = (HoveredEnemyIndex >= 0) ? HoveredEnemyIndex : (EnemyDice.Num() + HoveredModifierIndex);
		CurrentOption = (CurrentOption + 1) % TotalOptions;

		if (CurrentOption < EnemyDice.Num())
		{
			HoveredEnemyIndex = CurrentOption;
			HoveredModifierIndex = -1;
		}
		else
		{
			HoveredEnemyIndex = -1;
			HoveredModifierIndex = CurrentOption - EnemyDice.Num();
		}
	}
	UpdateSelectionHighlights();
}

void ADiceGameManager::OnSelectPrev()
{
	if (CurrentPhase != EGamePhase::PlayerMatching) return;

	if (SelectionMode == 0)
	{
		int32 StartIndex = SelectedDiceIndex;
		do {
			SelectedDiceIndex = (SelectedDiceIndex - 1 + PlayerDice.Num()) % PlayerDice.Num();
		} while (PlayerDiceMatched.IsValidIndex(SelectedDiceIndex) &&
				 PlayerDiceMatched[SelectedDiceIndex] &&
				 SelectedDiceIndex != StartIndex);

		SelectDice(SelectedDiceIndex);
	}
	else if (SelectionMode == 1)
	{
		int32 TotalOptions = EnemyDice.Num() + AllModifiers.Num();
		int32 CurrentOption = (HoveredEnemyIndex >= 0) ? HoveredEnemyIndex : (EnemyDice.Num() + HoveredModifierIndex);
		CurrentOption = (CurrentOption - 1 + TotalOptions) % TotalOptions;

		if (CurrentOption < EnemyDice.Num())
		{
			HoveredEnemyIndex = CurrentOption;
			HoveredModifierIndex = -1;
		}
		else
		{
			HoveredEnemyIndex = -1;
			HoveredModifierIndex = CurrentOption - EnemyDice.Num();
		}
	}
	UpdateSelectionHighlights();
}

void ADiceGameManager::OnConfirmSelection()
{
	if (CurrentPhase != EGamePhase::PlayerMatching) return;

	if (SelectionMode == 0 && SelectedDiceIndex >= 0)
	{
		SelectionMode = 1;
		HoveredEnemyIndex = 0;
		HoveredModifierIndex = -1;
		UpdateSelectionHighlights();
	}
	else if (SelectionMode == 1)
	{
		if (HoveredEnemyIndex >= 0)
		{
			TryMatchDice(SelectedDiceIndex, HoveredEnemyIndex);
		}
		else if (HoveredModifierIndex >= 0 && AllModifiers.IsValidIndex(HoveredModifierIndex))
		{
			TryApplyModifier(AllModifiers[HoveredModifierIndex]);
		}
	}
}

void ADiceGameManager::OnCancelSelection()
{
	if (CurrentPhase != EGamePhase::PlayerMatching) return;

	if (SelectionMode == 1)
	{
		SelectionMode = 0;
		HoveredEnemyIndex = -1;
		HoveredModifierIndex = -1;
		UpdateSelectionHighlights();
	}
}

void ADiceGameManager::UpdateDiceDebugVisibility()
{
	for (ADice* D : EnemyDice)
	{
		if (D) D->bShowDebugNumbers = bShowDebugGizmos;
	}
	for (ADice* D : PlayerDice)
	{
		if (D) D->bShowDebugNumbers = bShowDebugGizmos;
	}
}

void ADiceGameManager::StartGame()
{
	ClearAllDice();

	EnemyResults.Empty();
	PlayerResults.Empty();
	PlayerDiceMatched.Empty();
	EnemyDiceMatched.Empty();
	WaitTimer = 0.0f;
	SelectionMode = 0;
	SelectedDiceIndex = -1;
	HoveredEnemyIndex = -1;
	HoveredModifierIndex = -1;

	CurrentPhase = EGamePhase::EnemyThrowing;
	EnemyThrowDice();
}

void ADiceGameManager::EnemyThrowDice()
{
	AMaskEnemy* Enemy = FindEnemy();
	if (!Enemy) return;

	FVector EnemyLocation = Enemy->GetActorLocation();
	FVector SpawnBase = EnemyLocation + EnemyDiceSpawnOffset;

	ADiceCamera* Cam = FindCamera();
	FVector ThrowTarget = FVector::ZeroVector;
	if (Cam)
	{
		ThrowTarget = Cam->GetActorLocation() + FVector(0, 0, -100.0f);
	}

	for (int32 i = 0; i < EnemyNumDice; i++)
	{
		FVector SpawnOffset = FVector(
			FMath::RandRange(-15.0f, 15.0f),
			(i - EnemyNumDice / 2.0f) * 20.0f,
			FMath::RandRange(0.0f, 10.0f)
		);

		FVector SpawnLocation = SpawnBase + SpawnOffset;
		FRotator SpawnRotation = FRotator(
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f)
		);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ADice* NewDice = GetWorld()->SpawnActor<ADice>(ADice::StaticClass(), SpawnLocation, SpawnRotation, Params);
		if (NewDice)
		{
			NewDice->bShowDebugNumbers = bShowDebugGizmos;

			FVector ThrowDirection = (ThrowTarget - SpawnLocation).GetSafeNormal();
			ThrowDirection += FVector(
				FMath::RandRange(-0.15f, 0.15f),
				FMath::RandRange(-0.15f, 0.15f),
				FMath::RandRange(-0.1f, 0.0f)
			);

			NewDice->Throw(ThrowDirection, DiceThrowForce);
			EnemyDice.Add(NewDice);
		}
	}

	bEnemyDiceSettled = false;
	WaitTimer = 0.0f;
	CurrentPhase = EGamePhase::EnemyDiceSettling;
}

void ADiceGameManager::CheckEnemyDiceSettled(float DeltaTime)
{
	bool AllSettled = true;
	for (ADice* D : EnemyDice)
	{
		if (D && !D->IsStill())
		{
			AllSettled = false;
			break;
		}
	}

	if (AllSettled && EnemyDice.Num() > 0)
	{
		if (!bEnemyDiceSettled)
		{
			bEnemyDiceSettled = true;
			WaitTimer = 0.0f;
		}

		WaitTimer += DeltaTime;

		if (WaitTimer >= 1.5f)
		{
			PrepareEnemyDiceLineup();
			LineupProgress = 0.0f;
			CurrentPhase = EGamePhase::EnemyDiceLining;
		}
	}
}

void ADiceGameManager::PrepareEnemyDiceLineup()
{
	EnemyDiceStartPositions.Empty();
	EnemyDiceStartRotations.Empty();
	EnemyDiceTargetPositions.Empty();
	EnemyDiceTargetRotations.Empty();

	FVector CenterPos = FVector::ZeroVector;
	for (ADice* D : EnemyDice)
	{
		if (D) CenterPos += D->GetActorLocation();
	}
	if (EnemyDice.Num() > 0) CenterPos /= EnemyDice.Num();

	for (int32 i = 0; i < EnemyDice.Num(); i++)
	{
		ADice* D = EnemyDice[i];
		if (!D) continue;

		EnemyDiceStartPositions.Add(D->GetActorLocation());
		EnemyDiceStartRotations.Add(D->GetActorRotation());

		FVector TargetPos = CenterPos;
		TargetPos.Y = CenterPos.Y + (i - EnemyDice.Num() / 2.0f + 0.5f) * DiceLineupSpacing;
		TargetPos.Z = D->GetActorLocation().Z;
		EnemyDiceTargetPositions.Add(TargetPos);

		FRotator TargetRot = D->GetActorRotation();
		TargetRot.Roll = FMath::RoundToFloat(TargetRot.Roll / 90.0f) * 90.0f;
		TargetRot.Pitch = FMath::RoundToFloat(TargetRot.Pitch / 90.0f) * 90.0f;
		TargetRot.Yaw = FMath::RoundToFloat(TargetRot.Yaw / 90.0f) * 90.0f;
		EnemyDiceTargetRotations.Add(TargetRot);

		D->Mesh->SetSimulatePhysics(false);
	}
}

float ADiceGameManager::SmoothStep(float t)
{
	return t * t * (3.0f - 2.0f * t);
}

float ADiceGameManager::EaseOutCubic(float t)
{
	return 1.0f - FMath::Pow(1.0f - t, 3.0f);
}

void ADiceGameManager::LineUpEnemyDice(float DeltaTime)
{
	LineupProgress += DeltaTime * DiceLineupSpeed;
	bool AllDone = true;

	for (int32 i = 0; i < EnemyDice.Num(); i++)
	{
		ADice* D = EnemyDice[i];
		if (!D || i >= EnemyDiceStartPositions.Num()) continue;

		float DiceTime = LineupProgress - (i * StaggerDelay);
		float Alpha = FMath::Clamp(DiceTime, 0.0f, 1.0f);
		if (Alpha < 1.0f) AllDone = false;

		float SmoothAlpha = EaseOutCubic(Alpha);

		FVector NewPos = FMath::Lerp(EnemyDiceStartPositions[i], EnemyDiceTargetPositions[i], SmoothAlpha);
		D->SetActorLocation(NewPos);

		FRotator NewRot = FMath::Lerp(EnemyDiceStartRotations[i], EnemyDiceTargetRotations[i], SmoothAlpha);
		D->SetActorRotation(NewRot);
	}

	if (AllDone && LineupProgress >= 1.0f + (EnemyDice.Num() * StaggerDelay))
	{
		EnemyResults.Empty();
		for (ADice* D : EnemyDice)
		{
			if (D)
			{
				int32 Result = D->GetResult();
				EnemyResults.Add(Result);
				D->CurrentValue = Result;
			}
		}
		EnemyDiceMatched.Init(false, EnemyDice.Num());
		WaitTimer = 0.0f;
		StartPlayerTurn();
	}
}

void ADiceGameManager::StartPlayerTurn()
{
	CurrentPhase = EGamePhase::PlayerTurn;
}

void ADiceGameManager::PlayerThrowDice()
{
	for (ADice* D : PlayerDice)
	{
		if (D && IsValid(D)) D->Destroy();
	}
	PlayerDice.Empty();
	PlayerResults.Empty();

	ADiceCamera* Cam = FindCamera();
	if (!Cam) return;

	FVector CamLocation = Cam->Camera->GetComponentLocation();
	FVector CamForward = Cam->Camera->GetForwardVector();
	FVector CamRight = Cam->Camera->GetRightVector();
	FVector SpawnBase = CamLocation + CamForward * 50.0f - FVector(0, 0, 20.0f);

	CurrentPhase = EGamePhase::PlayerThrowing;

	for (int32 i = 0; i < PlayerNumDice; i++)
	{
		FVector Offset = CamRight * (i - PlayerNumDice / 2.0f) * 15.0f;
		Offset += FVector(
			FMath::RandRange(-5.0f, 5.0f),
			FMath::RandRange(-5.0f, 5.0f),
			FMath::RandRange(-5.0f, 5.0f)
		);

		FVector SpawnLocation = SpawnBase + Offset;
		FRotator SpawnRotation = FRotator(
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f)
		);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ADice* NewDice = GetWorld()->SpawnActor<ADice>(ADice::StaticClass(), SpawnLocation, SpawnRotation, Params);
		if (NewDice)
		{
			NewDice->bShowDebugNumbers = bShowDebugGizmos;

			FVector ThrowDirection = CamForward + FVector(0, 0, -0.3f);
			ThrowDirection += FVector(
				FMath::RandRange(-0.1f, 0.1f),
				FMath::RandRange(-0.1f, 0.1f),
				0
			);

			NewDice->Throw(ThrowDirection, DiceThrowForce);
			PlayerDice.Add(NewDice);
		}
	}

	bPlayerDiceSettled = false;
	WaitTimer = 0.0f;
	CurrentPhase = EGamePhase::PlayerDiceSettling;
}

void ADiceGameManager::CheckPlayerDiceSettled()
{
	bool AllSettled = true;
	for (ADice* D : PlayerDice)
	{
		if (D && !D->IsStill())
		{
			AllSettled = false;
			break;
		}
	}

	if (AllSettled && PlayerDice.Num() > 0)
	{
		if (!bPlayerDiceSettled)
		{
			bPlayerDiceSettled = true;
			WaitTimer = 0.0f;
		}

		WaitTimer += GetWorld()->GetDeltaSeconds();

		if (WaitTimer >= 1.5f)
		{
			PreparePlayerDiceLineup();
			PlayerLineupProgress = 0.0f;
			CurrentPhase = EGamePhase::PlayerDiceLining;
		}
	}
}

void ADiceGameManager::PreparePlayerDiceLineup()
{
	PlayerDiceStartPositions.Empty();
	PlayerDiceStartRotations.Empty();
	PlayerDiceTargetPositions.Empty();
	PlayerDiceTargetRotations.Empty();

	FVector CenterPos = FVector::ZeroVector;
	for (ADice* D : PlayerDice)
	{
		if (D) CenterPos += D->GetActorLocation();
	}
	if (PlayerDice.Num() > 0) CenterPos /= PlayerDice.Num();

	for (int32 i = 0; i < PlayerDice.Num(); i++)
	{
		ADice* D = PlayerDice[i];
		if (!D) continue;

		PlayerDiceStartPositions.Add(D->GetActorLocation());
		PlayerDiceStartRotations.Add(D->GetActorRotation());

		FVector TargetPos = CenterPos;
		TargetPos.Y = CenterPos.Y + (i - PlayerDice.Num() / 2.0f + 0.5f) * DiceLineupSpacing;
		TargetPos.Z = D->GetActorLocation().Z;
		PlayerDiceTargetPositions.Add(TargetPos);

		FRotator TargetRot = D->GetActorRotation();
		TargetRot.Roll = FMath::RoundToFloat(TargetRot.Roll / 90.0f) * 90.0f;
		TargetRot.Pitch = FMath::RoundToFloat(TargetRot.Pitch / 90.0f) * 90.0f;
		TargetRot.Yaw = FMath::RoundToFloat(TargetRot.Yaw / 90.0f) * 90.0f;
		PlayerDiceTargetRotations.Add(TargetRot);

		D->Mesh->SetSimulatePhysics(false);
	}
}

void ADiceGameManager::LineUpPlayerDice(float DeltaTime)
{
	PlayerLineupProgress += DeltaTime * DiceLineupSpeed;
	bool AllDone = true;

	for (int32 i = 0; i < PlayerDice.Num(); i++)
	{
		ADice* D = PlayerDice[i];
		if (!D || i >= PlayerDiceStartPositions.Num()) continue;

		float DiceTime = PlayerLineupProgress - (i * StaggerDelay);
		float Alpha = FMath::Clamp(DiceTime, 0.0f, 1.0f);
		if (Alpha < 1.0f) AllDone = false;

		float SmoothAlpha = EaseOutCubic(Alpha);

		FVector NewPos = FMath::Lerp(PlayerDiceStartPositions[i], PlayerDiceTargetPositions[i], SmoothAlpha);
		D->SetActorLocation(NewPos);

		FRotator NewRot = FMath::Lerp(PlayerDiceStartRotations[i], PlayerDiceTargetRotations[i], SmoothAlpha);
		D->SetActorRotation(NewRot);
	}

	if (AllDone && PlayerLineupProgress >= 1.0f + (PlayerDice.Num() * StaggerDelay))
	{
		PlayerResults.Empty();
		for (ADice* D : PlayerDice)
		{
			if (D)
			{
				int32 Result = D->GetResult();
				PlayerResults.Add(Result);
				D->CurrentValue = Result;
			}
		}
		PlayerDiceMatched.Init(false, PlayerDice.Num());
		StartMatchingPhase();
	}
}

void ADiceGameManager::StartMatchingPhase()
{
	CurrentPhase = EGamePhase::PlayerMatching;
	SelectionMode = 0;
	SelectedDiceIndex = 0;

	for (int32 i = 0; i < PlayerDiceMatched.Num(); i++)
	{
		if (!PlayerDiceMatched[i])
		{
			SelectedDiceIndex = i;
			break;
		}
	}

	SelectDice(SelectedDiceIndex);
	UpdateSelectionHighlights();
}

void ADiceGameManager::UpdateMatchingPhase()
{
	UpdateSelectionHighlights();
}

void ADiceGameManager::SelectDice(int32 Index)
{
	SelectedDiceIndex = Index;
	if (PlayerDice.IsValidIndex(Index))
	{
		SelectedPlayerDice = PlayerDice[Index];
	}
	else
	{
		SelectedPlayerDice = nullptr;
	}
}

void ADiceGameManager::UpdateSelectionHighlights()
{
	for (int32 i = 0; i < PlayerDice.Num(); i++)
	{
		if (PlayerDice[i])
		{
			bool bHighlight = (i == SelectedDiceIndex && SelectionMode == 0);
			PlayerDice[i]->SetHighlighted(bHighlight);
		}
	}

	for (int32 i = 0; i < EnemyDice.Num(); i++)
	{
		if (EnemyDice[i])
		{
			bool bHighlight = (i == HoveredEnemyIndex && SelectionMode == 1);
			EnemyDice[i]->SetHighlighted(bHighlight);
		}
	}

	for (int32 i = 0; i < AllModifiers.Num(); i++)
	{
		if (AllModifiers[i])
		{
			bool bHighlight = (i == HoveredModifierIndex && SelectionMode == 1);
			AllModifiers[i]->SetHighlighted(bHighlight);
		}
	}
}

void ADiceGameManager::TryMatchDice(int32 PlayerIndex, int32 EnemyIndex)
{
	if (!PlayerDice.IsValidIndex(PlayerIndex) || !EnemyDice.IsValidIndex(EnemyIndex)) return;
	if (PlayerDiceMatched[PlayerIndex] || EnemyDiceMatched[EnemyIndex]) return;

	int32 PlayerVal = PlayerResults[PlayerIndex];
	int32 EnemyVal = EnemyResults[EnemyIndex];

	if (PlayerVal == EnemyVal)
	{
		PlayerDiceMatched[PlayerIndex] = true;
		EnemyDiceMatched[EnemyIndex] = true;

		PlayerDice[PlayerIndex]->SetMatched(true);
		EnemyDice[EnemyIndex]->SetMatched(true);

		SelectionMode = 0;
		HoveredEnemyIndex = -1;
		HoveredModifierIndex = -1;

		CheckAllMatched();

		if (CurrentPhase == EGamePhase::PlayerMatching)
		{
			for (int32 i = 0; i < PlayerDiceMatched.Num(); i++)
			{
				if (!PlayerDiceMatched[i])
				{
					SelectedDiceIndex = i;
					SelectDice(i);
					break;
				}
			}
		}
		UpdateSelectionHighlights();
	}
}

void ADiceGameManager::TryApplyModifier(ADiceModifier* Modifier)
{
	if (!Modifier || Modifier->bIsUsed) return;
	if (!PlayerDice.IsValidIndex(SelectedDiceIndex)) return;
	if (PlayerDiceMatched[SelectedDiceIndex]) return;

	int32 OldValue = PlayerResults[SelectedDiceIndex];
	int32 NewValue = Modifier->ApplyToValue(OldValue);

	if (Modifier->ModifierType == EModifierType::RerollOne)
	{
		NewValue = FMath::RandRange(1, 6);
	}
	else if (Modifier->ModifierType == EModifierType::RerollAll)
	{
		for (int32 i = 0; i < PlayerResults.Num(); i++)
		{
			if (!PlayerDiceMatched[i])
			{
				int32 RerollValue = FMath::RandRange(1, 6);
				PlayerResults[i] = RerollValue;
				if (PlayerDice[i])
				{
					PlayerDice[i]->CurrentValue = RerollValue;
				}
			}
		}
		Modifier->UseModifier();
		SelectionMode = 0;
		HoveredEnemyIndex = -1;
		HoveredModifierIndex = -1;
		UpdateSelectionHighlights();
		return;
	}

	PlayerResults[SelectedDiceIndex] = NewValue;
	if (PlayerDice[SelectedDiceIndex])
	{
		PlayerDice[SelectedDiceIndex]->CurrentValue = NewValue;
	}

	Modifier->UseModifier();
	SelectionMode = 0;
	HoveredEnemyIndex = -1;
	HoveredModifierIndex = -1;
	UpdateSelectionHighlights();
}

void ADiceGameManager::CheckAllMatched()
{
	int32 MatchCount = 0;
	for (bool bMatched : EnemyDiceMatched)
	{
		if (bMatched) MatchCount++;
	}

	if (MatchCount >= EnemyDice.Num())
	{
		DealDamage(true);
		CheckGameOver();

		if (CurrentPhase != EGamePhase::GameOver)
		{
			CurrentPhase = EGamePhase::RoundEnd;
		}
	}
}

void ADiceGameManager::DealDamage(bool bToEnemy)
{
	if (bToEnemy)
	{
		EnemyHealth = FMath::Max(0, EnemyHealth - 1);
	}
	else
	{
		PlayerHealth = FMath::Max(0, PlayerHealth - 1);
	}
}

void ADiceGameManager::CheckGameOver()
{
	if (EnemyHealth <= 0 || PlayerHealth <= 0)
	{
		CurrentPhase = EGamePhase::GameOver;
	}
}

void ADiceGameManager::ClearAllDice()
{
	for (ADice* D : EnemyDice)
	{
		if (D && IsValid(D)) D->Destroy();
	}
	EnemyDice.Empty();

	for (ADice* D : PlayerDice)
	{
		if (D && IsValid(D)) D->Destroy();
	}
	PlayerDice.Empty();
}

void ADiceGameManager::DrawTurnText()
{
	FString Text;
	FColor Color = FColor::White;

	switch (CurrentPhase)
	{
		case EGamePhase::Idle:
			Text = "Press G to Start";
			Color = FColor::Yellow;
			break;
		case EGamePhase::EnemyThrowing:
			Text = "Enemy Throwing...";
			Color = FColor::Red;
			break;
		case EGamePhase::EnemyDiceSettling:
			Text = "Enemy Rolling...";
			Color = FColor::Red;
			break;
		case EGamePhase::EnemyDiceLining:
			Text = "Enemy Shows Hand";
			Color = FColor::Red;
			break;
		case EGamePhase::PlayerTurn:
			Text = "YOUR TURN - Press E!";
			Color = FColor::Green;
			break;
		case EGamePhase::PlayerThrowing:
			Text = "Throwing...";
			Color = FColor::Cyan;
			break;
		case EGamePhase::PlayerDiceSettling:
			Text = "Rolling...";
			Color = FColor::Cyan;
			break;
		case EGamePhase::PlayerDiceLining:
			Text = "Your Hand";
			Color = FColor::Cyan;
			break;
		case EGamePhase::PlayerMatching:
			if (SelectionMode == 0)
				Text = "Select YOUR dice (A/D) then SPACE";
			else
				Text = "Select TARGET (A/D) then SPACE | Q to cancel";
			Color = FColor::Yellow;
			break;
		case EGamePhase::RoundEnd:
			Text = "Round Won! Press G for Next Round";
			Color = FColor::Green;
			break;
		case EGamePhase::GameOver:
			if (EnemyHealth <= 0)
				Text = "YOU WIN! Press G to Restart";
			else
				Text = "GAME OVER - Press G to Restart";
			Color = (EnemyHealth <= 0) ? FColor::Green : FColor::Red;
			break;
	}

	GEngine->AddOnScreenDebugMessage(1, 0.0f, Color, Text, true, FVector2D(2.0f, 2.0f));

	if (EnemyResults.Num() > 0)
	{
		FString EnemyText = "Enemy: ";
		for (int32 i = 0; i < EnemyResults.Num(); i++)
		{
			bool bMatched = EnemyDiceMatched.IsValidIndex(i) && EnemyDiceMatched[i];
			EnemyText += bMatched ? FString::Printf(TEXT("X ")) : FString::Printf(TEXT("[%d] "), EnemyResults[i]);
		}
		GEngine->AddOnScreenDebugMessage(2, 0.0f, FColor::Red, EnemyText, true, FVector2D(1.5f, 1.5f));
	}

	if (PlayerResults.Num() > 0)
	{
		FString PlayerText = "You: ";
		for (int32 i = 0; i < PlayerResults.Num(); i++)
		{
			bool bMatched = PlayerDiceMatched.IsValidIndex(i) && PlayerDiceMatched[i];
			bool bSelected = (i == SelectedDiceIndex && SelectionMode == 0);
			if (bMatched)
				PlayerText += TEXT("X ");
			else if (bSelected)
				PlayerText += FString::Printf(TEXT(">[%d]< "), PlayerResults[i]);
			else
				PlayerText += FString::Printf(TEXT("[%d] "), PlayerResults[i]);
		}
		GEngine->AddOnScreenDebugMessage(3, 0.0f, FColor::Green, PlayerText, true, FVector2D(1.5f, 1.5f));
	}
}

void ADiceGameManager::DrawHealthBars()
{
	FString HealthText = FString::Printf(TEXT("Enemy HP: %d/%d | Your HP: %d/%d"), EnemyHealth, MaxHealth, PlayerHealth, MaxHealth);
	GEngine->AddOnScreenDebugMessage(4, 0.0f, FColor::White, HealthText, true, FVector2D(1.5f, 1.5f));
}

FRotator ADiceGameManager::GetRotationForFaceUp(int32 FaceValue)
{
	switch (FaceValue)
	{
		case 1: return FRotator(0, 0, 0);
		case 2: return FRotator(-90, 0, 0);
		case 3: return FRotator(0, 0, -90);
		case 4: return FRotator(0, 0, 90);
		case 5: return FRotator(90, 0, 0);
		case 6: return FRotator(180, 0, 0);
		default: return FRotator(0, 0, 0);
	}
}

AMaskEnemy* ADiceGameManager::FindEnemy()
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMaskEnemy::StaticClass(), Enemies);
	return (Enemies.Num() > 0) ? Cast<AMaskEnemy>(Enemies[0]) : nullptr;
}

ADiceCamera* ADiceGameManager::FindCamera()
{
	TArray<AActor*> Cameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADiceCamera::StaticClass(), Cameras);
	return (Cameras.Num() > 0) ? Cast<ADiceCamera>(Cameras[0]) : nullptr;
}
