#include "DiceGameManager.h"
#include "MaskEnemy.h"
#include "DiceCamera.h"
#include "PlayerHandComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"

ADiceGameManager::ADiceGameManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// Setup
	EnemyNumDice = 4;
	PlayerNumDice = 5;
	DiceThrowForce = 400.0f;
	EnemyDiceSpawnOffset = FVector(0.0f, 0.0f, 80.0f);
	TableActor = nullptr;

	// Dice visuals
	PlayerDiceMesh = nullptr;
	PlayerDiceMaterial = nullptr;
	EnemyDiceMesh = nullptr;
	EnemyDiceMaterial = nullptr;
	PlayerTextColor = FColor(255, 200, 50);  // Golden
	EnemyTextColor = FColor::White;
	bPlayerDiceGlow = true;
	DiceScale = 0.15f;
	CustomMeshScale = 1.0f;
	bShowDiceNumbers = true;
	DiceTextSize = 50.0f;
	DiceTextOffset = 51.0f;

	// Lineup - centered at origin
	LineupCenter = FVector(0.0f, 0.0f, 0.0f);
	LineupYaw = 0.0f;
	DiceLineupSpacing = 25.0f;
	EnemyRowOffset = 40.0f;   // Positive = behind center (enemy side)
	PlayerRowOffset = 40.0f;  // Positive = in front of center (player side)
	DiceLineupHeight = 10.0f;
	DiceLineupSpeed = 2.0f;
	StaggerDelay = 0.1f;

	// Camera - adjustable
	MatchCameraOffset = FVector(-150.0f, 0.0f, 200.0f);
	MatchCameraPitch = -55.0f;
	CameraPanSpeed = 2.0f;

	// Dragging
	DragHeight = 40.0f;
	DragFollowSpeed = 30.0f;
	DragTiltAmount = 3.0f;

	// Health
	MaxHealth = 5;
	PlayerHealth = 5;
	EnemyHealth = 5;

	// State
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

	// Adjust mode - disabled by default
	bAdjustMode = false;
	AdjustStep = 5.0f;
	AdjustSelection = 0;

	// Face rotation tool
	bFaceRotationMode = false;
	CurrentFaceEdit = 1;
	TestDice = nullptr;
	// Initialize face rotations (index 0 unused, 1-6 for faces)
	FaceRotations.SetNum(7);
	FaceRotations[1] = FRotator(0, 0, 0);
	FaceRotations[2] = FRotator(90, 180, 270);
	FaceRotations[3] = FRotator(-180, 90, 450);
	FaceRotations[4] = FRotator(-360, -180, 90);
	FaceRotations[5] = FRotator(-810, 15, 435);
	FaceRotations[6] = FRotator(180, -180, 0);

	// Dragging state
	bIsDragging = false;
	DraggedDice = nullptr;
	DraggedDiceIndex = -1;
	OriginalDragPosition = FVector::ZeroVector;
	OriginalDragRotation = FRotator::ZeroRotator;
	LastDragPosition = FVector::ZeroVector;

	// Return animation
	bDiceReturning = false;
	ReturningDice = nullptr;
	ReturningDiceIndex = -1;
	ReturnProgress = 0.0f;
	ReturnVelocity = FVector::ZeroVector;

	// Modifier snap animation
	bDiceSnappingToModifier = false;
	SnapTargetModifier = nullptr;
	SnapDiceIndex = -1;

	// Modifier effect animation
	bDiceFlipping = false;
	FlipDiceIndex = -1;
	FlipProgress = 0.0f;

	// Reroll from modifier
	bRerollAfterSnap = false;
	RerollDiceIndex = -1;
	bRerollAll = false;
	bWaitingForCameraToRerollAll = false;
	RerollAllWaitTimer = 0.0f;
	bDiceLiftingForReroll = false;
	DiceLiftProgress = 0.0f;

	// Match animation
	bMatchAnimating = false;
	MatchPlayerIndex = -1;
	MatchEnemyIndex = -1;
	MatchAnimProgress = 0.0f;

	// Camera
	CameraPanProgress = 0.0f;
	bCameraPanning = false;
	bCameraAtMatchView = false;
	OriginalCameraLocation = FVector::ZeroVector;
	OriginalCameraRotation = FRotator::ZeroRotator;

	// Modifier shuffle
	bModifierShuffling = false;
	ModifierShuffleProgress = 0.0f;
	FadingModifier = nullptr;
	FadingModifierAlpha = 1.0f;
	CurrentRound = 0;

	// Hands
	PlayerHandActor = nullptr;
	EnemyHandActor = nullptr;
	bWaitingForChop = false;
	bWaitingForCameraThenEnemyChop = false;

	// Dice disperse
	bDiceDispersing = false;
	DiceDisperseProgress = 0.0f;
}

void ADiceGameManager::BeginPlay()
{
	Super::BeginPlay();
	SetupInputBindings();
	FindAllModifiers();

	// Store initial camera position
	ADiceCamera* Cam = FindCamera();
	if (Cam)
	{
		OriginalCameraLocation = Cam->GetActorLocation();
		OriginalCameraRotation = Cam->GetActorRotation();
	}

	// Bind to hand chop events
	UPlayerHandComponent* PlayerHand = GetPlayerHand();
	if (PlayerHand)
	{
		PlayerHand->OnChopComplete.AddDynamic(this, &ADiceGameManager::OnPlayerChopComplete);
	}
	UPlayerHandComponent* EnemyHand = GetEnemyHand();
	if (EnemyHand)
	{
		EnemyHand->OnChopComplete.AddDynamic(this, &ADiceGameManager::OnEnemyChopComplete);
	}
}

void ADiceGameManager::SetupInputBindings()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		EnableInput(PC);
		if (InputComponent)
		{
			// E is now the main action key - starts game, throws dice, continues
			InputComponent->BindKey(EKeys::E, IE_Pressed, this, &ADiceGameManager::OnPlayerActionPressed);
			InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ADiceGameManager::OnStartGamePressed);  // Keep G as backup
			InputComponent->BindKey(EKeys::T, IE_Pressed, this, &ADiceGameManager::OnToggleDebugPressed);
			InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ADiceGameManager::OnToggleFaceRotationMode);
			InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ADiceGameManager::OnMousePressed);
			InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ADiceGameManager::OnMouseReleased);
			InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ADiceGameManager::OnGiveUpPressed);
		}
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
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

	if (bFaceRotationMode)
	{
		UpdateFaceRotationMode();
	}
	else if (bAdjustMode)
	{
		UpdateAdjustMode();
		DrawAdjustGizmos();
	}

	DrawTurnText();
	DrawHealthBars();

	// Always update camera pan (so it works during all phases)
	UpdateCameraPan(DeltaTime);

	// Update dice disperse animation
	UpdateDiceDisperse(DeltaTime);

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
			UpdateMouseInput();
			UpdateDiceReturn(DeltaTime);
			UpdateModifierSnap(DeltaTime);
			UpdateDiceFlip(DeltaTime);
			UpdateMatchAnimation(DeltaTime);
			UpdateModifierShuffle(DeltaTime);
			// Check if waiting for camera to reset before RE:ALL
			if (bWaitingForCameraToRerollAll)
			{
				RerollAllWaitTimer += DeltaTime;
				if (RerollAllWaitTimer >= 0.6f)  // Wait for camera to start moving
				{
					bWaitingForCameraToRerollAll = false;
					StartDiceLiftForReroll();
				}
			}
			// Animate dice lifting before throw
			if (bDiceLiftingForReroll)
			{
				UpdateDiceLiftForReroll(DeltaTime);
			}
			break;

		default:
			break;
	}
}

void ADiceGameManager::OnStartGamePressed()
{
	if (CurrentPhase == EGamePhase::Idle)
	{
		StartGame();
	}
	else if (CurrentPhase == EGamePhase::RoundEnd)
	{
		// Continue to next round (don't reset everything)
		CurrentRound++;
		ContinueToNextRound();
	}
	else if (CurrentPhase == EGamePhase::GameOver)
	{
		PlayerHealth = MaxHealth;
		EnemyHealth = MaxHealth;
		// Reset all modifiers for new game
		PermanentlyRemovedModifiers.Empty();
		for (ADiceModifier* Mod : AllModifiers)
		{
			if (Mod)
			{
				Mod->bIsUsed = false;
				Mod->ModifierText->SetVisibility(true);
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

void ADiceGameManager::OnPlayerActionPressed()
{
	// E key does different things based on game state
	if (bWaitingForChop)
	{
		// Don't allow actions while waiting for chop animation
		return;
	}

	if (CurrentPhase == EGamePhase::Idle)
	{
		// Start the game
		StartGame();
	}
	else if (CurrentPhase == EGamePhase::PlayerTurn)
	{
		// Throw player dice
		PlayerThrowDice();
	}
	else if (CurrentPhase == EGamePhase::RoundEnd)
	{
		// Continue to next round
		CurrentRound++;
		ContinueToNextRound();
	}
	else if (CurrentPhase == EGamePhase::GameOver)
	{
		// Restart game
		PlayerHealth = MaxHealth;
		EnemyHealth = MaxHealth;
		PermanentlyRemovedModifiers.Empty();
		for (ADiceModifier* Mod : AllModifiers)
		{
			if (Mod)
			{
				Mod->bIsUsed = false;
				Mod->ModifierText->SetVisibility(true);
			}
		}
		StartGame();
	}
}

void ADiceGameManager::OnToggleDebugPressed()
{
	bShowDebugGizmos = !bShowDebugGizmos;
	UpdateDiceDebugVisibility();
}

void ADiceGameManager::OnToggleFaceRotationMode()
{
	bFaceRotationMode = !bFaceRotationMode;
	if (bFaceRotationMode)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("Face rotation mode ON - Press 1-6 to select face"));
		SpawnTestDice();
	}
	else
	{
		if (TestDice)
		{
			TestDice->Destroy();
			TestDice = nullptr;
		}
	}
}

void ADiceGameManager::OnSelectNext() {}
void ADiceGameManager::OnSelectPrev() {}
void ADiceGameManager::OnConfirmSelection() {}
void ADiceGameManager::OnCancelSelection() {}

void ADiceGameManager::OnGiveUpPressed()
{
	if (CurrentPhase == EGamePhase::PlayerMatching)
	{
		// Only allow if not in the middle of animations
		if (!bDiceReturning && !bDiceSnappingToModifier && !bDiceFlipping && !bMatchAnimating && !bWaitingForCameraToRerollAll && !bDiceLiftingForReroll && !bIsDragging && !bModifierShuffling)
		{
			GiveUpRound();
		}
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
	PlayerDiceModified.Empty();
	PlayerDiceAtModifier.Empty();
	WaitTimer = 0.0f;
	SelectionMode = 0;
	SelectedDiceIndex = -1;

	// Reset round counter and permanently removed modifiers for new game
	CurrentRound = 1;
	PermanentlyRemovedModifiers.Empty();

	// Reset all modifiers to usable state for new game
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod)
		{
			Mod->bIsUsed = false;
			Mod->SetActive(false);
			// Make sure it's visible
			Mod->ModifierText->SetVisibility(true);
		}
	}

	// Ensure camera is at original position
	ResetCamera();

	CurrentPhase = EGamePhase::EnemyThrowing;
	EnemyThrowDice();
}

void ADiceGameManager::EnemyThrowDice()
{
	AMaskEnemy* Enemy = FindEnemy();
	if (!Enemy) return;

	FVector EnemyLocation = Enemy->GetActorLocation();
	FVector SpawnBase = EnemyLocation + EnemyDiceSpawnOffset;
	FVector ThrowTarget = GetLineupWorldCenter();

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
			NewDice->DiceSize = DiceScale;

			// Apply enemy dice visuals (use PlayerDiceMesh if no EnemyDiceMesh set)
			UStaticMesh* MeshToUse = EnemyDiceMesh ? EnemyDiceMesh : PlayerDiceMesh;
			if (MeshToUse)
			{
				NewDice->SetCustomMesh(MeshToUse, CustomMeshScale);
			}
			if (EnemyDiceMaterial)
			{
				NewDice->SetCustomMaterial(EnemyDiceMaterial);
			}
			NewDice->SetTextColor(EnemyTextColor);
			NewDice->SetFaceNumbersVisible(bShowDiceNumbers);
			NewDice->SetTextSettings(DiceTextSize, DiceTextOffset);
			NewDice->DiceSize = DiceScale;
			// Scale is handled in Dice::Tick with MeshNormalizeScale

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

	// Get base position from table if set
	FVector BaseCenter = LineupCenter;
	if (TableActor)
	{
		BaseCenter = TableActor->GetActorLocation() + LineupCenter;
	}

	// Rotation for lineup direction
	FRotator LineupRot = FRotator(0, LineupYaw, 0);
	FVector ForwardDir = LineupRot.RotateVector(FVector::ForwardVector);
	FVector RightDir = LineupRot.RotateVector(FVector::RightVector);

	for (int32 i = 0; i < EnemyDice.Num(); i++)
	{
		ADice* D = EnemyDice[i];
		if (!D) continue;

		EnemyDiceStartPositions.Add(D->GetActorLocation());
		EnemyDiceStartRotations.Add(D->GetActorRotation());

		// Center the row along right direction
		float RowWidth = (EnemyDice.Num() - 1) * DiceLineupSpacing;
		float DiceOffset = -RowWidth / 2.0f + (i * DiceLineupSpacing);

		FVector TargetPos = BaseCenter;
		// Enemy row is BEHIND center (positive forward from camera perspective)
		TargetPos += ForwardDir * FMath::Abs(EnemyRowOffset);
		TargetPos += RightDir * DiceOffset;
		TargetPos.Z = BaseCenter.Z + DiceLineupHeight;
		EnemyDiceTargetPositions.Add(TargetPos);

		int32 FaceValue = D->GetResult();
		FRotator TargetRot = GetRotationForFaceUp(FaceValue);
		// Add lineup yaw to existing rotation (don't overwrite)
		TargetRot.Yaw += LineupYaw;
		EnemyDiceTargetRotations.Add(TargetRot);

		D->Mesh->SetSimulatePhysics(false);
	}
}

float ADiceGameManager::EaseOutCubic(float t)
{
	return 1.0f - FMath::Pow(1.0f - t, 3.0f);
}

float ADiceGameManager::EaseOutElastic(float t)
{
	if (t == 0.0f || t == 1.0f) return t;
	float p = 0.4f;
	return FMath::Pow(2.0f, -10.0f * t) * FMath::Sin((t - p / 4.0f) * (2.0f * PI) / p) + 1.0f;
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
	FVector SpawnBase = CamLocation + CamForward * 80.0f + FVector(0, 0, 30.0f);

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
			NewDice->DiceSize = DiceScale;

			// Apply player dice visuals
			if (PlayerDiceMesh)
			{
				NewDice->SetCustomMesh(PlayerDiceMesh, CustomMeshScale);
			}
			if (PlayerDiceMaterial)
			{
				NewDice->SetCustomMaterial(PlayerDiceMaterial);
			}
			NewDice->SetTextColor(PlayerTextColor);
			NewDice->SetGlowEnabled(bPlayerDiceGlow);
			NewDice->SetFaceNumbersVisible(bShowDiceNumbers);
			NewDice->SetTextSettings(DiceTextSize, DiceTextOffset);
			NewDice->DiceSize = DiceScale;
			// Scale is handled in Dice::Tick with MeshNormalizeScale

			FVector ThrowDirection = CamForward + FVector(0, 0, 0.1f);
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
	// Only check dice that are currently being thrown (have physics enabled)
	bool AllSettled = true;
	for (int32 i = 0; i < PlayerDice.Num(); i++)
	{
		ADice* D = PlayerDice[i];
		if (!D) continue;

		// Skip dice that are matched or at a modifier (physics disabled)
		if (PlayerDiceMatched.IsValidIndex(i) && PlayerDiceMatched[i]) continue;
		if (PlayerDiceModified.IsValidIndex(i) && PlayerDiceModified[i]) continue;

		if (!D->IsStill())
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

		// Shorter wait for rerolls during matching phase
		float WaitTime = (PlayerDiceMatched.Num() > 0) ? 0.8f : 1.5f;

		if (WaitTimer >= WaitTime)
		{
			// Check if we're coming from a reroll during matching phase
			bool bWasReroll = (PlayerDiceMatched.Num() > 0);

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

	// Get base position from table if set
	FVector BaseCenter = LineupCenter;
	if (TableActor)
	{
		BaseCenter = TableActor->GetActorLocation() + LineupCenter;
	}

	// Rotation for lineup direction
	FRotator LineupRot = FRotator(0, LineupYaw, 0);
	FVector ForwardDir = LineupRot.RotateVector(FVector::ForwardVector);
	FVector RightDir = LineupRot.RotateVector(FVector::RightVector);

	for (int32 i = 0; i < PlayerDice.Num(); i++)
	{
		ADice* D = PlayerDice[i];
		if (!D) continue;

		// Skip dice that are at modifiers or matched - keep their current position
		bool bSkipLineup = false;
		if (PlayerDiceModified.IsValidIndex(i) && PlayerDiceModified[i] && PlayerDiceAtModifier[i])
		{
			// Keep at modifier position
			PlayerDiceStartPositions.Add(D->GetActorLocation());
			PlayerDiceStartRotations.Add(D->GetActorRotation());
			PlayerDiceTargetPositions.Add(D->GetActorLocation());
			PlayerDiceTargetRotations.Add(D->GetActorRotation());
			bSkipLineup = true;
		}
		else if (PlayerDiceMatched.IsValidIndex(i) && PlayerDiceMatched[i])
		{
			// Keep matched position
			PlayerDiceStartPositions.Add(D->GetActorLocation());
			PlayerDiceStartRotations.Add(D->GetActorRotation());
			PlayerDiceTargetPositions.Add(D->GetActorLocation());
			PlayerDiceTargetRotations.Add(D->GetActorRotation());
			bSkipLineup = true;
		}

		if (!bSkipLineup)
		{
			PlayerDiceStartPositions.Add(D->GetActorLocation());
			PlayerDiceStartRotations.Add(D->GetActorRotation());

			// Center the row along right direction
			float RowWidth = (PlayerDice.Num() - 1) * DiceLineupSpacing;
			float DiceOffset = -RowWidth / 2.0f + (i * DiceLineupSpacing);

			FVector TargetPos = BaseCenter;
			// Player row is IN FRONT of center (negative forward from camera perspective)
			TargetPos -= ForwardDir * FMath::Abs(PlayerRowOffset);
			TargetPos += RightDir * DiceOffset;
			TargetPos.Z = BaseCenter.Z + DiceLineupHeight;
			PlayerDiceTargetPositions.Add(TargetPos);

			int32 FaceValue = D->GetResult();
			FRotator TargetRot = GetRotationForFaceUp(FaceValue);
			// Add lineup yaw to existing rotation (don't overwrite)
			TargetRot.Yaw += LineupYaw;
			PlayerDiceTargetRotations.Add(TargetRot);
		}

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
		// Check if this is a reroll (matched arrays already exist)
		bool bIsReroll = (PlayerDiceMatched.Num() > 0);

		if (bIsReroll)
		{
			// Only update values for non-matched, non-modified dice
			for (int32 i = 0; i < PlayerDice.Num(); i++)
			{
				ADice* D = PlayerDice[i];
				if (!D) continue;
				if (PlayerDiceMatched[i] || PlayerDiceModified[i]) continue;

				int32 Result = D->GetResult();
				PlayerResults[i] = Result;
				D->CurrentValue = Result;
			}
			// Go back to matching phase
			CurrentPhase = EGamePhase::PlayerMatching;
			ActivateModifiers();
			// Pan camera back to closeup
			StartCameraPan();
		}
		else
		{
			// Initial lineup - set up all arrays
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
			PlayerDiceModified.Init(false, PlayerDice.Num());
			PlayerDiceAtModifier.Init(nullptr, PlayerDice.Num());
			StartMatchingPhase();
		}
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

	// Start juicy modifier shuffle animation (will activate when done)
	StartModifierShuffle();

	StartCameraPan();
}

void ADiceGameManager::UpdateMatchingPhase()
{
	// Handled in UpdateMouseInput
}

void ADiceGameManager::SelectDice(int32 Index)
{
	SelectedDiceIndex = Index;
	SelectedPlayerDice = PlayerDice.IsValidIndex(Index) ? PlayerDice[Index] : nullptr;
}

void ADiceGameManager::UpdateSelectionHighlights() {}

void ADiceGameManager::TryMatchDice(int32 PlayerIndex, int32 EnemyIndex)
{
	if (!PlayerDice.IsValidIndex(PlayerIndex) || !EnemyDice.IsValidIndex(EnemyIndex)) return;
	if (PlayerDiceMatched[PlayerIndex] || EnemyDiceMatched[EnemyIndex]) return;

	int32 PlayerVal = PlayerResults[PlayerIndex];
	int32 EnemyVal = EnemyResults[EnemyIndex];

	if (PlayerVal == EnemyVal)
	{
		// Start Balatro-style match animation
		StartMatchAnimation(PlayerIndex, EnemyIndex);
	}
}

void ADiceGameManager::StartMatchAnimation(int32 PlayerIdx, int32 EnemyIdx)
{
	if (!PlayerDice.IsValidIndex(PlayerIdx) || !EnemyDice.IsValidIndex(EnemyIdx)) return;

	bMatchAnimating = true;
	MatchPlayerIndex = PlayerIdx;
	MatchEnemyIndex = EnemyIdx;
	MatchAnimProgress = 0.0f;

	MatchStartPos = PlayerDice[PlayerIdx]->GetActorLocation();
	// Target is next to enemy dice
	MatchTargetPos = EnemyDice[EnemyIdx]->GetActorLocation();
	MatchTargetPos.Z = MatchStartPos.Z;  // Same height
}

void ADiceGameManager::UpdateMatchAnimation(float DeltaTime)
{
	if (!bMatchAnimating) return;

	if (!PlayerDice.IsValidIndex(MatchPlayerIndex) || !EnemyDice.IsValidIndex(MatchEnemyIndex))
	{
		bMatchAnimating = false;
		return;
	}

	ADice* PlayerD = PlayerDice[MatchPlayerIndex];
	ADice* EnemyD = EnemyDice[MatchEnemyIndex];
	if (!PlayerD || !EnemyD)
	{
		bMatchAnimating = false;
		return;
	}

	MatchAnimProgress += DeltaTime * 4.0f;
	float Alpha = FMath::Clamp(MatchAnimProgress, 0.0f, 1.0f);

	// Juicy ease
	float SmoothAlpha = EaseOutCubic(Alpha);

	// Player dice flies to enemy dice with arc
	FVector NewPos = FMath::Lerp(MatchStartPos, MatchTargetPos, SmoothAlpha);
	float ArcHeight = FMath::Sin(Alpha * PI) * 20.0f;
	NewPos.Z += ArcHeight;
	PlayerD->SetActorLocation(NewPos);

	// Scale pop effect on both dice
	float ScalePop = 1.0f + FMath::Sin(Alpha * PI) * 0.2f;
	PlayerD->Mesh->SetWorldScale3D(FVector(PlayerD->DiceSize * PlayerD->MeshNormalizeScale * ScalePop));
	EnemyD->Mesh->SetWorldScale3D(FVector(EnemyD->DiceSize * EnemyD->MeshNormalizeScale * ScalePop));

	if (MatchAnimProgress >= 1.0f)
	{
		// Animation complete - finalize match
		PlayerDiceMatched[MatchPlayerIndex] = true;
		EnemyDiceMatched[MatchEnemyIndex] = true;

		PlayerD->SetMatched(true);
		EnemyD->SetMatched(true);

		// Stack them together
		FVector FinalPos = EnemyD->GetActorLocation();
		FinalPos.Z += 8.0f;  // Stack on top
		PlayerD->SetActorLocation(FinalPos);

		// Reset scales
		PlayerD->Mesh->SetWorldScale3D(FVector(PlayerD->DiceSize * PlayerD->MeshNormalizeScale));
		EnemyD->Mesh->SetWorldScale3D(FVector(EnemyD->DiceSize * EnemyD->MeshNormalizeScale));

		// Juice: Small camera shake on match
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			PC->ClientStartCameraShake(nullptr, 0.3f);  // Use built-in if available
			// Or manual shake via rotation
			FRotator Shake = FRotator(
				FMath::RandRange(-0.5f, 0.5f),
				FMath::RandRange(-0.5f, 0.5f),
				0
			);
			PC->SetControlRotation(PC->GetControlRotation() + Shake);
		}

		bMatchAnimating = false;
		MatchPlayerIndex = -1;
		MatchEnemyIndex = -1;

		CheckAllMatched();
	}
}

void ADiceGameManager::TryApplyModifier(ADiceModifier* Modifier, int32 DiceIndex)
{
	if (!Modifier || Modifier->bIsUsed) return;
	if (!PlayerDice.IsValidIndex(DiceIndex)) return;
	if (PlayerDiceMatched[DiceIndex]) return;
	// REMOVED: PlayerDiceModified check - allow modifier combos!

	int32 OldValue = PlayerResults[DiceIndex];
	int32 NewValue = Modifier->ApplyToValue(OldValue);

	// Check if modifier would have no effect or invalid
	if (Modifier->ModifierType == EModifierType::MinusOne && OldValue <= 1)
	{
		// Can't go below 1 - reject
		return;
	}
	if (Modifier->ModifierType == EModifierType::PlusOne && OldValue >= 6)
	{
		// Can't go above 6 - reject
		return;
	}
	if (Modifier->ModifierType == EModifierType::PlusTwo && OldValue >= 6)
	{
		// Can't go above 6 - reject (even +1 would cap at 6)
		return;
	}

	// Handle RE:1 - snap to modifier then throw
	if (Modifier->ModifierType == EModifierType::RerollOne)
	{
		PlayerDiceModified[DiceIndex] = true;
		PlayerDiceAtModifier[DiceIndex] = Modifier;
		bRerollAfterSnap = true;
		bRerollAll = false;
		RerollDiceIndex = DiceIndex;
		SnapDiceToModifier(DiceIndex, Modifier);
		Modifier->UseModifier();
		return;
	}

	// Handle RE:ALL - reset camera first, then throw all
	if (Modifier->ModifierType == EModifierType::RerollAll)
	{
		// Start camera reset, wait, then throw
		bWaitingForCameraToRerollAll = true;
		RerollAllWaitTimer = 0.0f;
		DeactivateModifiers();
		ResetCamera();
		Modifier->UseModifier();
		return;
	}

	// Handle FLIP - juicy flip animation
	if (Modifier->ModifierType == EModifierType::Flip)
	{
		PlayerResults[DiceIndex] = NewValue;
		PlayerDice[DiceIndex]->CurrentValue = NewValue;
		PlayerDiceModified[DiceIndex] = true;
		PlayerDiceAtModifier[DiceIndex] = Modifier;
		SnapDiceToModifier(DiceIndex, Modifier);
		// Start flip after snap completes (handled in UpdateModifierSnap)
		Modifier->UseModifier();
		return;
	}

	// Handle +1, -1, +2 - snap and rotate to new value
	PlayerResults[DiceIndex] = NewValue;
	PlayerDice[DiceIndex]->CurrentValue = NewValue;
	PlayerDiceModified[DiceIndex] = true;
	PlayerDiceAtModifier[DiceIndex] = Modifier;
	SnapDiceToModifier(DiceIndex, Modifier);

	Modifier->UseModifier();
}

void ADiceGameManager::UpdateDiceFaceDisplay(ADice* Dice, int32 NewValue)
{
	if (!Dice) return;

	// Rotate the dice to show the new value facing up
	FRotator NewRot = GetRotationForFaceUp(NewValue);
	NewRot.Yaw += LineupYaw;
	Dice->SetActorRotation(NewRot);
}

void ADiceGameManager::SnapDiceToModifier(int32 DiceIndex, ADiceModifier* Modifier)
{
	if (!Modifier || !PlayerDice.IsValidIndex(DiceIndex)) return;

	ADice* Dice = PlayerDice[DiceIndex];
	if (!Dice) return;

	// Start snap animation
	bDiceSnappingToModifier = true;
	SnapTargetModifier = Modifier;
	SnapDiceIndex = DiceIndex;
	ReturnProgress = 0.0f;
	ReturnStartPos = Dice->GetActorLocation();
	ReturnStartRot = Dice->GetActorRotation();

	// Target position is above the modifier
	ReturnTargetPos = Modifier->GetActorLocation() + FVector(0, 0, 20.0f);
	ReturnTargetRot = GetRotationForFaceUp(PlayerResults[DiceIndex]);
	ReturnTargetRot.Yaw += LineupYaw;
}

void ADiceGameManager::UpdateModifierSnap(float DeltaTime)
{
	if (!bDiceSnappingToModifier) return;
	if (!PlayerDice.IsValidIndex(SnapDiceIndex))
	{
		bDiceSnappingToModifier = false;
		return;
	}

	ADice* Dice = PlayerDice[SnapDiceIndex];
	if (!Dice)
	{
		bDiceSnappingToModifier = false;
		return;
	}

	ReturnProgress += DeltaTime * 5.0f;  // Faster snap
	float Alpha = FMath::Clamp(ReturnProgress, 0.0f, 1.0f);

	// Smooth easing
	float SmoothAlpha = EaseOutCubic(Alpha);

	// Lerp position only during snap (rotation happens after for FLIP)
	FVector NewPos = FMath::Lerp(ReturnStartPos, ReturnTargetPos, SmoothAlpha);
	Dice->SetActorLocation(NewPos);

	// Check if this is a FLIP modifier - don't rotate during snap
	bool bIsFlip = (SnapTargetModifier && SnapTargetModifier->ModifierType == EModifierType::Flip);
	if (!bIsFlip)
	{
		FRotator NewRot = FMath::Lerp(ReturnStartRot, ReturnTargetRot, SmoothAlpha);
		Dice->SetActorRotation(NewRot);
	}

	if (ReturnProgress >= 1.0f)
	{
		Dice->SetActorLocation(ReturnTargetPos);

		int32 CompletedIndex = SnapDiceIndex;
		ADiceModifier* CompletedModifier = SnapTargetModifier;

		bDiceSnappingToModifier = false;
		SnapTargetModifier = nullptr;
		SnapDiceIndex = -1;

		// After snap completes, handle special effects
		if (bRerollAfterSnap && !bRerollAll && RerollDiceIndex == CompletedIndex)
		{
			// RE:1 - now throw this dice
			bRerollAfterSnap = false;
			RerollSingleDice(CompletedIndex);
		}
		else if (bIsFlip)
		{
			// FLIP - start juicy flip animation
			StartDiceFlip(CompletedIndex, PlayerResults[CompletedIndex]);
		}
		else
		{
			// +1/-1/+2 - set final rotation
			Dice->SetActorRotation(ReturnTargetRot);
		}
	}
}

void ADiceGameManager::StartDiceFlip(int32 DiceIndex, int32 NewValue)
{
	if (!PlayerDice.IsValidIndex(DiceIndex)) return;

	ADice* Dice = PlayerDice[DiceIndex];
	if (!Dice) return;

	bDiceFlipping = true;
	FlipDiceIndex = DiceIndex;
	FlipProgress = 0.0f;
	FlipStartRot = Dice->GetActorRotation();

	// Target rotation shows the new (flipped) value
	FlipTargetRot = GetRotationForFaceUp(NewValue);
	FlipTargetRot.Yaw += LineupYaw;
}

void ADiceGameManager::UpdateDiceFlip(float DeltaTime)
{
	if (!bDiceFlipping) return;
	if (!PlayerDice.IsValidIndex(FlipDiceIndex))
	{
		bDiceFlipping = false;
		return;
	}

	ADice* Dice = PlayerDice[FlipDiceIndex];
	if (!Dice)
	{
		bDiceFlipping = false;
		return;
	}

	FlipProgress += DeltaTime * 2.5f;  // Juicy speed
	float Alpha = FMath::Clamp(FlipProgress, 0.0f, 1.0f);

	// Juicy flip - overshoot then settle
	float FlipAlpha = EaseOutElastic(Alpha);

	// Add a hop during flip
	FVector BasePos = Dice->GetActorLocation();
	float HopHeight = FMath::Sin(Alpha * PI) * 15.0f;
	Dice->SetActorLocation(FVector(BasePos.X, BasePos.Y, PlayerDiceAtModifier[FlipDiceIndex]->GetActorLocation().Z + 20.0f + HopHeight));

	// Rotate with extra spin for juice
	FRotator CurrentRot = FMath::Lerp(FlipStartRot, FlipTargetRot, FlipAlpha);
	// Add extra roll wobble
	CurrentRot.Roll += FMath::Sin(Alpha * PI * 3.0f) * (1.0f - Alpha) * 15.0f;
	Dice->SetActorRotation(CurrentRot);

	if (FlipProgress >= 1.0f)
	{
		// Final position and rotation
		Dice->SetActorLocation(FVector(BasePos.X, BasePos.Y, PlayerDiceAtModifier[FlipDiceIndex]->GetActorLocation().Z + 20.0f));
		Dice->SetActorRotation(FlipTargetRot);
		bDiceFlipping = false;
		FlipDiceIndex = -1;
	}
}

void ADiceGameManager::RerollSingleDice(int32 DiceIndex)
{
	if (!PlayerDice.IsValidIndex(DiceIndex)) return;

	ADice* Dice = PlayerDice[DiceIndex];
	if (!Dice) return;

	// Store modifier position for throw direction
	FVector ModPos = Dice->GetActorLocation();

	// Clear modified state so it returns to lineup
	PlayerDiceModified[DiceIndex] = false;
	PlayerDiceAtModifier[DiceIndex] = nullptr;

	// Add random rotation for drama
	Dice->SetActorRotation(FRotator(
		FMath::RandRange(0.0f, 360.0f),
		FMath::RandRange(0.0f, 360.0f),
		FMath::RandRange(0.0f, 360.0f)
	));

	// Re-enable physics
	Dice->Mesh->SetSimulatePhysics(true);
	Dice->bHasBeenThrown = false;

	// Throw upward and toward lineup center with spin
	FVector ThrowTarget = GetLineupWorldCenter();
	FVector ThrowDir = (ThrowTarget - ModPos).GetSafeNormal();
	ThrowDir.Z = 0.8f;  // Mostly upward
	ThrowDir.Normalize();
	ThrowDir += FVector(
		FMath::RandRange(-0.15f, 0.15f),
		FMath::RandRange(-0.15f, 0.15f),
		0
	);

	Dice->Throw(ThrowDir, DiceThrowForce * 0.6f);

	// Mark that we need to wait for this dice to settle
	bPlayerDiceSettled = false;
	WaitTimer = 0.0f;

	// Go back to settling phase
	CurrentPhase = EGamePhase::PlayerDiceSettling;
}

void ADiceGameManager::StartDiceLiftForReroll()
{
	RerollStartPositions.Empty();
	RerollLiftPositions.Empty();

	// Calculate lift positions for all unmatched dice
	for (int32 i = 0; i < PlayerDice.Num(); i++)
	{
		if (PlayerDiceMatched[i])
		{
			RerollStartPositions.Add(FVector::ZeroVector);
			RerollLiftPositions.Add(FVector::ZeroVector);
			continue;
		}

		ADice* Dice = PlayerDice[i];
		if (!Dice)
		{
			RerollStartPositions.Add(FVector::ZeroVector);
			RerollLiftPositions.Add(FVector::ZeroVector);
			continue;
		}

		FVector StartPos = Dice->GetActorLocation();
		RerollStartPositions.Add(StartPos);

		// Lift position - up and slightly back
		FVector LiftPos = StartPos;
		LiftPos.Z += 100.0f;  // Go up high
		LiftPos += FVector(FMath::RandRange(-30.0f, 30.0f), FMath::RandRange(-30.0f, 30.0f), 0);
		RerollLiftPositions.Add(LiftPos);

		// Clear modified state
		PlayerDiceModified[i] = false;
		PlayerDiceAtModifier[i] = nullptr;
	}

	bDiceLiftingForReroll = true;
	DiceLiftProgress = 0.0f;
}

void ADiceGameManager::UpdateDiceLiftForReroll(float DeltaTime)
{
	DiceLiftProgress += DeltaTime * 2.0f;  // Smooth lift speed
	float Alpha = FMath::Clamp(DiceLiftProgress, 0.0f, 1.0f);
	float SmoothAlpha = EaseOutCubic(Alpha);

	// Animate all unmatched dice lifting
	for (int32 i = 0; i < PlayerDice.Num(); i++)
	{
		if (PlayerDiceMatched[i]) continue;

		ADice* Dice = PlayerDice[i];
		if (!Dice) continue;

		if (RerollStartPositions.IsValidIndex(i) && RerollLiftPositions.IsValidIndex(i))
		{
			FVector NewPos = FMath::Lerp(RerollStartPositions[i], RerollLiftPositions[i], SmoothAlpha);
			Dice->SetActorLocation(NewPos);

			// Add some rotation during lift
			FRotator CurrentRot = Dice->GetActorRotation();
			CurrentRot.Yaw += DeltaTime * 90.0f;
			CurrentRot.Pitch += DeltaTime * 60.0f;
			Dice->SetActorRotation(CurrentRot);
		}
	}

	// When lift is done, throw them down
	if (DiceLiftProgress >= 1.0f)
	{
		bDiceLiftingForReroll = false;
		RerollAllUnmatchedDice();
	}
}

void ADiceGameManager::RerollAllUnmatchedDice()
{
	bool bAnyRerolled = false;

	// Get lineup center for throwing toward
	FVector ThrowTarget = GetLineupWorldCenter();

	// Throw from current (lifted) positions
	for (int32 i = 0; i < PlayerDice.Num(); i++)
	{
		// Skip matched dice
		if (PlayerDiceMatched[i]) continue;

		ADice* Dice = PlayerDice[i];
		if (!Dice) continue;

		FVector SpawnPos = Dice->GetActorLocation();

		// Random rotation for drama
		Dice->SetActorRotation(FRotator(
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f)
		));

		// Re-enable physics
		Dice->Mesh->SetSimulatePhysics(true);
		Dice->bHasBeenThrown = false;

		// Throw DOWNWARD toward table - like initial throw
		FVector ThrowDir = (ThrowTarget - SpawnPos).GetSafeNormal();
		ThrowDir += FVector(
			FMath::RandRange(-0.15f, 0.15f),
			FMath::RandRange(-0.15f, 0.15f),
			FMath::RandRange(-0.1f, 0.0f)
		);

		Dice->Throw(ThrowDir, DiceThrowForce);
		bAnyRerolled = true;
	}

	if (bAnyRerolled)
	{
		// Go back to settling phase to wait for dice
		bPlayerDiceSettled = false;
		WaitTimer = 0.0f;
		bRerollAfterSnap = false;
		bRerollAll = false;
		CurrentPhase = EGamePhase::PlayerDiceSettling;
	}
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
		DeactivateModifiers();
		ResetCamera();

		// Clear dice
		ClearAllDice();

		EnemyResults.Empty();
		PlayerResults.Empty();
		PlayerDiceMatched.Empty();
		EnemyDiceMatched.Empty();
		PlayerDiceModified.Empty();
		PlayerDiceAtModifier.Empty();
		WaitTimer = 0.0f;
		SelectionMode = 0;
		SelectedDiceIndex = -1;

		// Wait for camera to finish panning, then chop enemy finger
		bWaitingForCameraThenEnemyChop = true;
	}
}

void ADiceGameManager::DealDamage(bool bToEnemy)
{
	if (bToEnemy)
		EnemyHealth = FMath::Max(0, EnemyHealth - 1);
	else
		PlayerHealth = FMath::Max(0, PlayerHealth - 1);
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
	// Start disperse animation instead of immediate destroy
	StartDiceDisperse();
}

void ADiceGameManager::StartDiceDisperse()
{
	DispersingDice.Empty();
	DisperseStartPositions.Empty();
	DisperseVelocities.Empty();
	DisperseStartScales.Empty();

	// Gather all dice for dispersing
	FVector Center = GetLineupWorldCenter();

	for (ADice* D : EnemyDice)
	{
		if (D && IsValid(D))
		{
			DispersingDice.Add(D);
			DisperseStartPositions.Add(D->GetActorLocation());
			DisperseStartScales.Add(D->DiceSize * D->MeshNormalizeScale);

			// Random outward velocity with upward arc
			FVector ToCenter = (D->GetActorLocation() - Center).GetSafeNormal();
			FVector Velocity = ToCenter * FMath::RandRange(150.0f, 300.0f);
			Velocity.Z = FMath::RandRange(100.0f, 200.0f);
			Velocity += FVector(FMath::RandRange(-50.0f, 50.0f), FMath::RandRange(-50.0f, 50.0f), 0);
			DisperseVelocities.Add(Velocity);

			// Disable physics so we control the animation
			D->Mesh->SetSimulatePhysics(false);
		}
	}

	for (ADice* D : PlayerDice)
	{
		if (D && IsValid(D))
		{
			DispersingDice.Add(D);
			DisperseStartPositions.Add(D->GetActorLocation());
			DisperseStartScales.Add(D->DiceSize * D->MeshNormalizeScale);

			// Random outward velocity with upward arc
			FVector ToCenter = (D->GetActorLocation() - Center).GetSafeNormal();
			FVector Velocity = ToCenter * FMath::RandRange(150.0f, 300.0f);
			Velocity.Z = FMath::RandRange(100.0f, 200.0f);
			Velocity += FVector(FMath::RandRange(-50.0f, 50.0f), FMath::RandRange(-50.0f, 50.0f), 0);
			DisperseVelocities.Add(Velocity);

			// Disable physics so we control the animation
			D->Mesh->SetSimulatePhysics(false);
		}
	}

	EnemyDice.Empty();
	PlayerDice.Empty();

	if (DispersingDice.Num() > 0)
	{
		bDiceDispersing = true;
		DiceDisperseProgress = 0.0f;
	}
}

void ADiceGameManager::UpdateDiceDisperse(float DeltaTime)
{
	if (!bDiceDispersing) return;

	DiceDisperseProgress += DeltaTime * 2.0f;  // Animation speed
	float Alpha = FMath::Clamp(DiceDisperseProgress, 0.0f, 1.0f);

	// Ease out for smooth deceleration
	float EasedAlpha = 1.0f - FMath::Pow(1.0f - Alpha, 2.0f);

	for (int32 i = 0; i < DispersingDice.Num(); i++)
	{
		ADice* D = DispersingDice[i];
		if (!D || !IsValid(D)) continue;

		// Move along velocity with gravity
		FVector NewPos = DisperseStartPositions[i] + DisperseVelocities[i] * EasedAlpha;
		// Add gravity curve
		NewPos.Z -= 100.0f * Alpha * Alpha;
		D->SetActorLocation(NewPos);

		// Spin wildly
		FRotator CurrentRot = D->GetActorRotation();
		float SpinSpeed = (1.0f - Alpha) * 500.0f;  // Slow down over time
		CurrentRot.Pitch += DeltaTime * SpinSpeed * (i % 2 == 0 ? 1.0f : -1.0f);
		CurrentRot.Yaw += DeltaTime * SpinSpeed * 0.7f;
		CurrentRot.Roll += DeltaTime * SpinSpeed * 1.3f;
		D->SetActorRotation(CurrentRot);

		// Shrink and fade
		float Scale = DisperseStartScales[i] * (1.0f - Alpha * 0.8f);  // Shrink to 20%
		D->Mesh->SetWorldScale3D(FVector(Scale));

		// Fade out text
		for (UTextRenderComponent* Text : D->FaceTexts)
		{
			if (Text)
			{
				FColor TextColor = D->TextColor;
				TextColor.A = FMath::Clamp(int32(255 * (1.0f - Alpha)), 0, 255);
				Text->SetTextRenderColor(TextColor);
			}
		}
	}

	// When animation complete, destroy all
	if (DiceDisperseProgress >= 1.0f)
	{
		for (ADice* D : DispersingDice)
		{
			if (D && IsValid(D))
			{
				D->Destroy();
			}
		}
		DispersingDice.Empty();
		DisperseStartPositions.Empty();
		DisperseVelocities.Empty();
		DisperseStartScales.Empty();
		bDiceDispersing = false;
	}
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
		case EGamePhase::PlayerDiceSettling:
			Text = "Rolling...";
			Color = FColor::Cyan;
			break;
		case EGamePhase::PlayerDiceLining:
			Text = "Your Hand";
			Color = FColor::Cyan;
			break;
		case EGamePhase::PlayerMatching:
			if (bModifierShuffling)
			{
				Text = "Modifiers shuffling...";
				Color = FColor::Cyan;
			}
			else if (bIsDragging)
			{
				Text = "Drop on matching dice!";
				Color = FColor::Yellow;
			}
			else if (!CanStillMatch())
			{
				Text = "NO MATCHES! Press SPACE to take damage";
				Color = FColor::Red;
			}
			else
			{
				Text = "Drag your dice! (SPACE to give up)";
				Color = FColor::Yellow;
			}
			break;
		case EGamePhase::RoundEnd:
			Text = "Round Won! Press G";
			Color = FColor::Green;
			break;
		case EGamePhase::GameOver:
			Text = (EnemyHealth <= 0) ? "YOU WIN! Press G" : "GAME OVER - Press G";
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
			EnemyText += bMatched ? TEXT("X ") : FString::Printf(TEXT("[%d] "), EnemyResults[i]);
		}
		GEngine->AddOnScreenDebugMessage(2, 0.0f, FColor::Red, EnemyText, true, FVector2D(1.5f, 1.5f));
	}

	if (PlayerResults.Num() > 0)
	{
		FString PlayerText = "You: ";
		for (int32 i = 0; i < PlayerResults.Num(); i++)
		{
			bool bMatched = PlayerDiceMatched.IsValidIndex(i) && PlayerDiceMatched[i];
			bool bModified = PlayerDiceModified.IsValidIndex(i) && PlayerDiceModified[i];
			if (bMatched)
			{
				PlayerText += TEXT("X ");
			}
			else if (bModified)
			{
				// Show modified dice with asterisk
				PlayerText += FString::Printf(TEXT("[%d*] "), PlayerResults[i]);
			}
			else
			{
				PlayerText += FString::Printf(TEXT("[%d] "), PlayerResults[i]);
			}
		}
		GEngine->AddOnScreenDebugMessage(3, 0.0f, FColor::Green, PlayerText, true, FVector2D(1.5f, 1.5f));
	}
}

void ADiceGameManager::DrawHealthBars()
{
	FString HealthText = FString::Printf(TEXT("Round %d | Enemy HP: %d/%d | Your HP: %d/%d"), CurrentRound, EnemyHealth, MaxHealth, PlayerHealth, MaxHealth);
	GEngine->AddOnScreenDebugMessage(4, 0.0f, FColor::White, HealthText, true, FVector2D(1.5f, 1.5f));
}

FRotator ADiceGameManager::GetRotationForFaceUp(int32 FaceValue)
{
	// Use the FaceRotations array (same one used by F mode tool)
	if (FaceValue >= 1 && FaceValue <= 6)
	{
		return FaceRotations[FaceValue];
	}
	return FRotator(0, 0, 0);
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

// ==================== MOUSE INPUT ====================

void ADiceGameManager::OnMousePressed()
{
	if (CurrentPhase != EGamePhase::PlayerMatching) return;
	if (bDiceReturning || bDiceSnappingToModifier || bDiceFlipping || bMatchAnimating || bWaitingForCameraToRerollAll || bDiceLiftingForReroll || bModifierShuffling) return;

	FVector HitLocation;
	AActor* HitActor = GetActorUnderMouse(HitLocation);
	if (!HitActor) return;

	ADice* HitDice = Cast<ADice>(HitActor);
	if (HitDice)
	{
		for (int32 i = 0; i < PlayerDice.Num(); i++)
		{
			if (PlayerDice[i] == HitDice && !PlayerDiceMatched[i])
			{
				StartDragging(HitDice, i);
				return;
			}
		}
	}
}

void ADiceGameManager::OnMouseReleased()
{
	if (!bIsDragging || CurrentPhase != EGamePhase::PlayerMatching) return;
	if (!DraggedDice) return;

	FVector DicePos = DraggedDice->GetActorLocation();
	bool bSuccess = false;
	bool bAppliedModifier = false;

	// Check enemy dice FIRST for matching (priority over modifiers)
	float ClosestEnemyDist = 60.0f;
	int32 ClosestEnemy = -1;

	for (int32 i = 0; i < EnemyDice.Num(); i++)
	{
		if (EnemyDice[i] && !EnemyDiceMatched[i])
		{
			float Dist = FVector::Dist(DicePos, EnemyDice[i]->GetActorLocation());
			if (Dist < ClosestEnemyDist)
			{
				ClosestEnemyDist = Dist;
				ClosestEnemy = i;
			}
		}
	}

	if (ClosestEnemy >= 0)
	{
		TryMatchDice(DraggedDiceIndex, ClosestEnemy);
		bSuccess = PlayerDiceMatched[DraggedDiceIndex] || bMatchAnimating;
	}

	// Check modifiers only if not matching enemy dice (combos allowed!)
	if (!bSuccess)
	{
		float ClosestModDist = 70.0f;
		ADiceModifier* ClosestMod = nullptr;

		for (ADiceModifier* Mod : AllModifiers)
		{
			if (Mod && !Mod->bIsUsed && Mod->bIsActive)
			{
				float Dist = FVector::Dist2D(DicePos, Mod->GetActorLocation());
				if (Dist < ClosestModDist)
				{
					ClosestModDist = Dist;
					ClosestMod = Mod;
				}
			}
		}

		if (ClosestMod)
		{
			// Check if modifier can actually be applied to this dice value
			int32 DiceValue = PlayerResults[DraggedDiceIndex];
			if (ClosestMod->CanApplyToValue(DiceValue))
			{
				TryApplyModifier(ClosestMod, DraggedDiceIndex);
				bAppliedModifier = true;
				bSuccess = true;
			}
			// If can't apply, bSuccess stays false and dice will bounce back
		}
	}

	// Handle result
	if (bAppliedModifier)
	{
		// Modifier handles the snap animation
		DraggedDice->SetHighlighted(false);
		DraggedDice->bIsBeingDragged = false;
		ClearAllHighlights();
		bIsDragging = false;
		DraggedDice = nullptr;
		DraggedDiceIndex = -1;
	}
	else if (bSuccess)
	{
		// Match success - snap back
		StopDragging(true);
	}
	else
	{
		// No hit - physics bounce back
		PhysicsBounceBack();
	}
}

void ADiceGameManager::UpdateMouseInput()
{
	if (bIsDragging)
	{
		UpdateDragging();
		HighlightValidTargets();
	}
	else if (!bDiceReturning && !bDiceSnappingToModifier && !bDiceFlipping && !bMatchAnimating && !bDiceLiftingForReroll && !bModifierShuffling)
	{
		FVector HitLocation;
		AActor* HitActor = GetActorUnderMouse(HitLocation);

		// Find which player dice (if any) is being hovered
		ADice* NewHoveredDice = nullptr;
		if (HitActor)
		{
			ADice* HitDice = Cast<ADice>(HitActor);
			if (HitDice)
			{
				for (int32 i = 0; i < PlayerDice.Num(); i++)
				{
					if (PlayerDice[i] == HitDice && !PlayerDiceMatched[i])
					{
						NewHoveredDice = HitDice;
						break;
					}
				}
			}
		}

		// Only update highlights if hovered dice changed
		for (ADice* D : PlayerDice)
		{
			if (D && !D->bIsMatched)
			{
				bool bShouldHighlight = (D == NewHoveredDice);
				if (D->bIsHighlighted != bShouldHighlight)
				{
					D->SetHighlighted(bShouldHighlight);
				}
			}
		}
	}
}

AActor* ADiceGameManager::GetActorUnderMouse(FVector& HitLocation)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return nullptr;

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
		return nullptr;

	FHitResult HitResult;
	FVector TraceEnd = WorldLocation + WorldDirection * 10000.0f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (DraggedDice) Params.AddIgnoredActor(DraggedDice);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldLocation, TraceEnd, ECC_Visibility, Params))
	{
		HitLocation = HitResult.Location;
		return HitResult.GetActor();
	}

	return nullptr;
}

FVector ADiceGameManager::GetMouseWorldPosition()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return FVector::ZeroVector;

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
		return FVector::ZeroVector;

	// Intersect with a plane at table level
	float PlaneZ = bIsDragging ? (OriginalDragPosition.Z + DragHeight) : DiceLineupHeight;
	if (FMath::Abs(WorldDirection.Z) > KINDA_SMALL_NUMBER)
	{
		float T = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
		if (T > 0)
		{
			return WorldLocation + WorldDirection * T;
		}
	}

	return WorldLocation + WorldDirection * 300.0f;
}

void ADiceGameManager::StartDragging(ADice* Dice, int32 Index)
{
	bIsDragging = true;
	DraggedDice = Dice;
	DraggedDiceIndex = Index;

	// Get the base position/rotation (before hover) if available
	if (Dice->bHighlightRotSet)
	{
		OriginalDragPosition = Dice->BaseHighlightPos;
		OriginalDragRotation = Dice->BaseHighlightRot;
		Dice->SetActorLocation(Dice->BaseHighlightPos);
		Dice->SetActorRotation(Dice->BaseHighlightRot);
	}
	else if (PlayerDiceModified.IsValidIndex(Index) && PlayerDiceModified[Index] && PlayerDiceAtModifier[Index])
	{
		// If dice is at a modifier, return position is at the modifier
		OriginalDragPosition = PlayerDiceAtModifier[Index]->GetActorLocation() + FVector(0, 0, 20.0f);
		OriginalDragRotation = Dice->GetActorRotation();
	}
	else
	{
		OriginalDragPosition = Dice->GetActorLocation();
		OriginalDragRotation = Dice->GetActorRotation();
	}

	// Mark as being dragged (stops hover in Tick)
	Dice->bIsBeingDragged = true;
	Dice->bHighlightRotSet = false;
	Dice->SetHighlighted(false);

	LastDragPosition = Dice->GetActorLocation();
}

void ADiceGameManager::PhysicsBounceBack()
{
	if (!DraggedDice)
	{
		bIsDragging = false;
		return;
	}

	DraggedDice->SetHighlighted(false);
	DraggedDice->bIsBeingDragged = false;
	ClearAllHighlights();

	// Enable physics for juicy bounce
	DraggedDice->Mesh->SetSimulatePhysics(true);

	// Calculate direction back to original position
	FVector CurrentPos = DraggedDice->GetActorLocation();
	FVector ToOrigin = OriginalDragPosition - CurrentPos;
	float Distance = ToOrigin.Size();

	// Give it a gentle push toward origin with some upward arc
	if (Distance > 1.0f)
	{
		FVector ImpulseDir = ToOrigin.GetSafeNormal();
		ImpulseDir.Z += 0.3f;  // Add upward arc
		ImpulseDir.Normalize();

		float ImpulseStrength = FMath::Clamp(Distance * 2.0f, 50.0f, 200.0f);
		DraggedDice->Mesh->AddImpulse(ImpulseDir * ImpulseStrength, NAME_None, true);

		// Add some spin for juice
		FVector RandomSpin = FVector(
			FMath::RandRange(-50.0f, 50.0f),
			FMath::RandRange(-50.0f, 50.0f),
			FMath::RandRange(-30.0f, 30.0f)
		);
		DraggedDice->Mesh->AddAngularImpulseInDegrees(RandomSpin, NAME_None, true);
	}

	// Start tracking return (we'll re-settle and re-lineup this dice)
	bDiceReturning = true;
	ReturningDice = DraggedDice;
	ReturningDiceIndex = DraggedDiceIndex;
	ReturnProgress = 0.0f;

	bIsDragging = false;
	DraggedDice = nullptr;
	DraggedDiceIndex = -1;
}

void ADiceGameManager::StopDragging(bool bSuccess)
{
	if (!DraggedDice)
	{
		bIsDragging = false;
		return;
	}

	DraggedDice->SetHighlighted(false);
	DraggedDice->bIsBeingDragged = false;
	ClearAllHighlights();

	if (bSuccess)
	{
		// Success - snap back
		DraggedDice->SetActorLocation(OriginalDragPosition);
		DraggedDice->SetActorRotation(OriginalDragRotation);
		bIsDragging = false;
		DraggedDice = nullptr;
		DraggedDiceIndex = -1;
	}
	else
	{
		// Fail - start juicy return
		bDiceReturning = true;
		ReturningDice = DraggedDice;
		ReturningDiceIndex = DraggedDiceIndex;
		ReturnProgress = 0.0f;
		ReturnStartPos = DraggedDice->GetActorLocation();
		ReturnStartRot = DraggedDice->GetActorRotation();
		ReturnTargetPos = OriginalDragPosition;
		ReturnTargetRot = OriginalDragRotation;

		// Calculate release velocity for physics feel
		FVector CurrentPos = DraggedDice->GetActorLocation();
		ReturnVelocity = (CurrentPos - LastDragPosition) / FMath::Max(GetWorld()->GetDeltaSeconds(), 0.001f);

		bIsDragging = false;
		DraggedDice = nullptr;
		DraggedDiceIndex = -1;
	}
}

void ADiceGameManager::UpdateDragging()
{
	if (!DraggedDice) return;

	FVector MouseWorld = GetMouseWorldPosition();
	FVector TargetPosition = MouseWorld;
	// Lift dice above its original position
	TargetPosition.Z = OriginalDragPosition.Z + DragHeight;

	FVector CurrentPos = DraggedDice->GetActorLocation();
	LastDragPosition = CurrentPos;

	// Direct mouse follow with smoothing
	FVector NewPosition = FMath::VInterpTo(CurrentPos, TargetPosition, GetWorld()->GetDeltaSeconds(), DragFollowSpeed);
	DraggedDice->SetActorLocation(NewPosition);

	// Tilt based on velocity
	FVector Velocity = (NewPosition - CurrentPos) / FMath::Max(GetWorld()->GetDeltaSeconds(), 0.001f);

	FRotator TargetRot = OriginalDragRotation;
	TargetRot.Roll += FMath::Clamp(Velocity.Y * DragTiltAmount * 0.01f, -20.0f, 20.0f);
	TargetRot.Pitch += FMath::Clamp(-Velocity.X * DragTiltAmount * 0.01f, -20.0f, 20.0f);

	FRotator CurrentRot = DraggedDice->GetActorRotation();
	FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRot, GetWorld()->GetDeltaSeconds(), 12.0f);
	DraggedDice->SetActorRotation(NewRot);
}

void ADiceGameManager::UpdateDiceReturn(float DeltaTime)
{
	if (!bDiceReturning || !ReturningDice) return;

	// Check if physics is enabled (bounce back mode)
	if (ReturningDice->Mesh->IsSimulatingPhysics())
	{
		// Wait for dice to settle
		ReturnProgress += DeltaTime;

		FVector Vel = ReturningDice->Mesh->GetPhysicsLinearVelocity();
		FVector AngVel = ReturningDice->Mesh->GetPhysicsAngularVelocityInDegrees();

		bool bSettled = (Vel.Size() < 5.0f && AngVel.Size() < 5.0f);
		bool bTimedOut = (ReturnProgress > 3.0f);  // Max 3 seconds

		if (bSettled || bTimedOut)
		{
			// Disable physics and smoothly move to lineup position
			ReturningDice->Mesh->SetSimulatePhysics(false);

			ReturnStartPos = ReturningDice->GetActorLocation();
			ReturnStartRot = ReturningDice->GetActorRotation();
			ReturnTargetPos = OriginalDragPosition;
			ReturnTargetRot = OriginalDragRotation;
			ReturnProgress = 0.0f;
		}
		return;
	}

	// Smooth animation back to lineup
	ReturnProgress += DeltaTime * 4.0f;
	float Alpha = FMath::Clamp(ReturnProgress, 0.0f, 1.0f);

	// Smooth ease
	float SmoothAlpha = EaseOutCubic(Alpha);

	FVector NewPos = FMath::Lerp(ReturnStartPos, ReturnTargetPos, SmoothAlpha);
	FRotator NewRot = FMath::Lerp(ReturnStartRot, ReturnTargetRot, SmoothAlpha);

	ReturningDice->SetActorLocation(NewPos);
	ReturningDice->SetActorRotation(NewRot);

	if (ReturnProgress >= 1.0f)
	{
		ReturningDice->SetActorLocation(ReturnTargetPos);
		ReturningDice->SetActorRotation(ReturnTargetRot);
		bDiceReturning = false;
		ReturningDice = nullptr;
		ReturningDiceIndex = -1;
	}
}

void ADiceGameManager::HighlightValidTargets()
{
	if (!DraggedDice || DraggedDiceIndex < 0) return;

	int32 DraggedValue = PlayerResults[DraggedDiceIndex];

	// Highlight enemy dice with matching values
	for (int32 i = 0; i < EnemyDice.Num(); i++)
	{
		if (EnemyDice[i] && !EnemyDiceMatched[i])
		{
			bool bCanMatch = (EnemyResults[i] == DraggedValue);
			EnemyDice[i]->SetHighlighted(bCanMatch);
		}
	}

	// Highlight modifiers - green if valid, red if invalid for this dice value
	// Combos allowed! Dice can use multiple modifiers
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && !Mod->bIsUsed && Mod->bIsActive)
		{
			if (!Mod->CanApplyToValue(DraggedValue))
			{
				// Can't apply this modifier to current value (e.g., +1 on 6)
				Mod->SetHighlighted(false);
				Mod->SetInvalid(true);
			}
			else
			{
				// Valid modifier for this dice
				Mod->SetHighlighted(true);
				Mod->SetInvalid(false);
			}
		}
	}
}

void ADiceGameManager::ClearAllHighlights()
{
	for (ADice* D : PlayerDice)
	{
		if (D && !D->bIsMatched) D->SetHighlighted(false);
	}
	for (ADice* D : EnemyDice)
	{
		if (D && !D->bIsMatched) D->SetHighlighted(false);
	}
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod)
		{
			Mod->SetHighlighted(false);
			Mod->SetInvalid(false);
		}
	}
}

void ADiceGameManager::PlayMatchEffect(FVector Location)
{
	// Particles handled elsewhere - this is just a hook for future VFX
}

// ==================== MODIFIERS ====================

void ADiceGameManager::ActivateModifiers()
{
	for (ADiceModifier* Mod : AllModifiers)
	{
		// Skip permanently removed modifiers
		if (Mod && !Mod->bIsUsed && !PermanentlyRemovedModifiers.Contains(Mod))
		{
			Mod->SetActive(true);
		}
	}
}

void ADiceGameManager::DeactivateModifiers()
{
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod)
		{
			Mod->SetActive(false);
		}
	}
}

// ==================== CAMERA ====================

void ADiceGameManager::StartCameraPan()
{
	ADiceCamera* Cam = FindCamera();
	if (!Cam) return;

	if (!bCameraAtMatchView)
	{
		OriginalCameraLocation = Cam->GetActorLocation();
		OriginalCameraRotation = Cam->GetActorRotation();
	}

	CameraPanProgress = 0.0f;
	bCameraPanning = true;
	bCameraAtMatchView = true;
}

FVector ADiceGameManager::GetLineupWorldCenter()
{
	FVector BaseCenter = LineupCenter;
	if (TableActor)
	{
		BaseCenter = TableActor->GetActorLocation() + LineupCenter;
	}
	return BaseCenter;
}

void ADiceGameManager::UpdateCameraPan(float DeltaTime)
{
	if (!bCameraPanning) return;

	ADiceCamera* Cam = FindCamera();
	if (!Cam) return;

	CameraPanProgress += DeltaTime * CameraPanSpeed;
	float Alpha = FMath::Clamp(CameraPanProgress, 0.0f, 1.0f);
	float SmoothAlpha = EaseOutCubic(Alpha);

	FVector TargetLoc;
	FRotator TargetRot;

	if (bCameraAtMatchView)
	{
		// Rotate camera offset by lineup yaw
		FRotator LineupRot = FRotator(0, LineupYaw, 0);
		FVector RotatedOffset = LineupRot.RotateVector(MatchCameraOffset);
		TargetLoc = GetLineupWorldCenter() + RotatedOffset;
		TargetRot = FRotator(MatchCameraPitch, LineupYaw, 0);
	}
	else
	{
		TargetLoc = OriginalCameraLocation;
		TargetRot = OriginalCameraRotation;
	}

	FVector NewLoc = FMath::Lerp(Cam->GetActorLocation(), TargetLoc, SmoothAlpha * 0.12f);
	FRotator NewRot = FMath::Lerp(Cam->GetActorRotation(), TargetRot, SmoothAlpha * 0.12f);

	Cam->SetActorLocation(NewLoc);
	Cam->SetActorRotation(NewRot);

	if (CameraPanProgress >= 2.0f)
	{
		bCameraPanning = false;

		// If we were waiting for camera to finish before enemy chop, do it now
		if (bWaitingForCameraThenEnemyChop)
		{
			bWaitingForCameraThenEnemyChop = false;
			TriggerEnemyChop();
		}
	}
}

void ADiceGameManager::ResetCamera()
{
	bCameraAtMatchView = false;
	CameraPanProgress = 0.0f;
	bCameraPanning = true;
}

// ==================== ADJUST MODE ====================

void ADiceGameManager::UpdateAdjustMode()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// Tab to cycle selection
	if (PC->WasInputKeyJustPressed(EKeys::Tab))
	{
		AdjustSelection = (AdjustSelection + 1) % 5;
	}

	// Arrow keys to adjust
	float Delta = AdjustStep;
	if (PC->IsInputKeyDown(EKeys::LeftShift)) Delta *= 3.0f;

	bool bChanged = false;

	if (PC->WasInputKeyJustPressed(EKeys::Up) || PC->WasInputKeyJustPressed(EKeys::W))
	{
		switch (AdjustSelection)
		{
			case 0: LineupCenter.X += Delta; break;
			case 1: EnemyRowOffset += Delta; break;
			case 2: PlayerRowOffset += Delta; break;
			case 3: DiceLineupSpacing += Delta; break;
			case 4: LineupYaw += Delta; break;
		}
		bChanged = true;
	}
	if (PC->WasInputKeyJustPressed(EKeys::Down) || PC->WasInputKeyJustPressed(EKeys::S))
	{
		switch (AdjustSelection)
		{
			case 0: LineupCenter.X -= Delta; break;
			case 1: EnemyRowOffset -= Delta; break;
			case 2: PlayerRowOffset -= Delta; break;
			case 3: DiceLineupSpacing -= Delta; break;
			case 4: LineupYaw -= Delta; break;
		}
		bChanged = true;
	}
	if (PC->WasInputKeyJustPressed(EKeys::Left) || PC->WasInputKeyJustPressed(EKeys::A))
	{
		switch (AdjustSelection)
		{
			case 0: LineupCenter.Y -= Delta; break;
			default: break;
		}
		bChanged = true;
	}
	if (PC->WasInputKeyJustPressed(EKeys::Right) || PC->WasInputKeyJustPressed(EKeys::D))
	{
		switch (AdjustSelection)
		{
			case 0: LineupCenter.Y += Delta; break;
			default: break;
		}
		bChanged = true;
	}
	if (PC->WasInputKeyJustPressed(EKeys::Q))
	{
		LineupCenter.Z -= Delta;
		bChanged = true;
	}
	if (PC->WasInputKeyJustPressed(EKeys::E))
	{
		LineupCenter.Z += Delta;
		bChanged = true;
	}

	// P to print settings
	if (PC->WasInputKeyJustPressed(EKeys::P))
	{
		PrintCurrentSettings();
	}

	// R to reset/refresh dice positions
	if (PC->WasInputKeyJustPressed(EKeys::R) && bChanged)
	{
		// Refresh lineup if dice exist
		if (EnemyDice.Num() > 0)
		{
			PrepareEnemyDiceLineup();
			for (int32 i = 0; i < EnemyDice.Num(); i++)
			{
				if (EnemyDice[i] && EnemyDiceTargetPositions.IsValidIndex(i))
				{
					EnemyDice[i]->SetActorLocation(EnemyDiceTargetPositions[i]);
					if (EnemyDiceTargetRotations.IsValidIndex(i))
						EnemyDice[i]->SetActorRotation(EnemyDiceTargetRotations[i]);
				}
			}
		}
		if (PlayerDice.Num() > 0)
		{
			PreparePlayerDiceLineup();
			for (int32 i = 0; i < PlayerDice.Num(); i++)
			{
				if (PlayerDice[i] && PlayerDiceTargetPositions.IsValidIndex(i))
				{
					PlayerDice[i]->SetActorLocation(PlayerDiceTargetPositions[i]);
					if (PlayerDiceTargetRotations.IsValidIndex(i))
						PlayerDice[i]->SetActorRotation(PlayerDiceTargetRotations[i]);
				}
			}
		}
	}
}

void ADiceGameManager::DrawAdjustGizmos()
{
	FVector Center = GetLineupWorldCenter();
	FRotator LineupRot = FRotator(0, LineupYaw, 0);
	FVector ForwardDir = LineupRot.RotateVector(FVector::ForwardVector);
	FVector RightDir = LineupRot.RotateVector(FVector::RightVector);

	// Draw center point
	FColor CenterColor = (AdjustSelection == 0) ? FColor::Yellow : FColor::White;
	DrawDebugSphere(GetWorld(), Center, 10.0f, 8, CenterColor, false, 0.0f);
	DrawDebugLine(GetWorld(), Center, Center + ForwardDir * 50.0f, FColor::Red, false, 0.0f, 0, 2.0f);
	DrawDebugLine(GetWorld(), Center, Center + RightDir * 50.0f, FColor::Green, false, 0.0f, 0, 2.0f);

	// Draw enemy row line
	FVector EnemyCenter = Center + ForwardDir * FMath::Abs(EnemyRowOffset);
	EnemyCenter.Z = Center.Z + DiceLineupHeight;
	FColor EnemyColor = (AdjustSelection == 1) ? FColor::Yellow : FColor::Red;
	float HalfWidth = (FMath::Max(EnemyNumDice, PlayerNumDice) - 1) * DiceLineupSpacing * 0.5f + 20.0f;
	DrawDebugLine(GetWorld(), EnemyCenter - RightDir * HalfWidth, EnemyCenter + RightDir * HalfWidth, EnemyColor, false, 0.0f, 0, 3.0f);

	// Draw player row line
	FVector PlayerCenter = Center - ForwardDir * FMath::Abs(PlayerRowOffset);
	PlayerCenter.Z = Center.Z + DiceLineupHeight;
	FColor PlayerColor = (AdjustSelection == 2) ? FColor::Yellow : FColor::Cyan;
	DrawDebugLine(GetWorld(), PlayerCenter - RightDir * HalfWidth, PlayerCenter + RightDir * HalfWidth, PlayerColor, false, 0.0f, 0, 3.0f);

	// Draw dice slot markers
	FColor SpacingColor = (AdjustSelection == 3) ? FColor::Yellow : FColor::Magenta;
	for (int32 i = 0; i < EnemyNumDice; i++)
	{
		float Offset = -((EnemyNumDice - 1) * DiceLineupSpacing * 0.5f) + (i * DiceLineupSpacing);
		FVector SlotPos = EnemyCenter + RightDir * Offset;
		DrawDebugBox(GetWorld(), SlotPos, FVector(5.0f), SpacingColor, false, 0.0f);
	}
	for (int32 i = 0; i < PlayerNumDice; i++)
	{
		float Offset = -((PlayerNumDice - 1) * DiceLineupSpacing * 0.5f) + (i * DiceLineupSpacing);
		FVector SlotPos = PlayerCenter + RightDir * Offset;
		DrawDebugBox(GetWorld(), SlotPos, FVector(5.0f), SpacingColor, false, 0.0f);
	}

	// HUD text
	TArray<FString> Options = { TEXT("Center XYZ"), TEXT("Enemy Row"), TEXT("Player Row"), TEXT("Spacing"), TEXT("Yaw") };
	TArray<FString> Values = {
		FString::Printf(TEXT("(%.0f, %.0f, %.0f)"), LineupCenter.X, LineupCenter.Y, LineupCenter.Z),
		FString::Printf(TEXT("%.0f"), EnemyRowOffset),
		FString::Printf(TEXT("%.0f"), PlayerRowOffset),
		FString::Printf(TEXT("%.0f"), DiceLineupSpacing),
		FString::Printf(TEXT("%.0f")	, LineupYaw)
	};

	GEngine->AddOnScreenDebugMessage(100, 0.0f, FColor::Cyan, TEXT("=== ADJUST MODE (Tab=cycle, Arrows=adjust, P=print, R=refresh) ==="));
	for (int32 i = 0; i < Options.Num(); i++)
	{
		FColor Col = (i == AdjustSelection) ? FColor::Yellow : FColor::White;
		FString Marker = (i == AdjustSelection) ? TEXT(">> ") : TEXT("   ");
		GEngine->AddOnScreenDebugMessage(101 + i, 0.0f, Col, FString::Printf(TEXT("%s%s: %s"), *Marker, *Options[i], *Values[i]));
	}
}

void ADiceGameManager::PrintCurrentSettings()
{
	UE_LOG(LogTemp, Warning, TEXT("=== LINEUP SETTINGS ==="));
	UE_LOG(LogTemp, Warning, TEXT("LineupCenter = FVector(%.1ff, %.1ff, %.1ff);"), LineupCenter.X, LineupCenter.Y, LineupCenter.Z);
	UE_LOG(LogTemp, Warning, TEXT("LineupYaw = %.1ff;"), LineupYaw);
	UE_LOG(LogTemp, Warning, TEXT("EnemyRowOffset = %.1ff;"), EnemyRowOffset);
	UE_LOG(LogTemp, Warning, TEXT("PlayerRowOffset = %.1ff;"), PlayerRowOffset);
	UE_LOG(LogTemp, Warning, TEXT("DiceLineupSpacing = %.1ff;"), DiceLineupSpacing);
	UE_LOG(LogTemp, Warning, TEXT("DiceLineupHeight = %.1ff;"), DiceLineupHeight);
	UE_LOG(LogTemp, Warning, TEXT("========================"));

	// Also show on screen
	GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, TEXT("Settings printed to Output Log!"));
}

// ==================== FACE ROTATION TOOL ====================

void ADiceGameManager::SpawnTestDice()
{
	if (TestDice)
	{
		TestDice->Destroy();
		TestDice = nullptr;
	}


	FVector SpawnLoc = GetLineupWorldCenter() + FVector(0, 0, 50.0f);
	FActorSpawnParameters Params;
	TestDice = GetWorld()->SpawnActor<ADice>(ADice::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params);
	if (TestDice)
	{
		TestDice->Mesh->SetSimulatePhysics(false);
		TestDice->DiceSize = DiceScale * 2.0f; // Make it bigger for visibility
		TestDice->bShowDebugNumbers = true;
		// Scale is handled in Dice::Tick with MeshNormalizeScale
	}
}

void ADiceGameManager::UpdateTestDice()
{
	if (!TestDice) return;

	FRotator Rot = FaceRotations[CurrentFaceEdit];
	// Add lineup yaw to match how gameplay applies it
	Rot.Yaw += LineupYaw;
	TestDice->SetActorRotation(Rot);
	TestDice->SetActorLocation(GetLineupWorldCenter() + FVector(0, 0, 50.0f));
}

// ==================== MATCH DETECTION ====================

bool ADiceGameManager::CanStillMatch()
{
	// Get available (unused, active) modifiers that could help
	TArray<ADiceModifier*> AvailableModifiers;
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && !Mod->bIsUsed && Mod->bIsActive)
		{
			AvailableModifiers.Add(Mod);
		}
	}

	// Check each unmatched player dice
	for (int32 p = 0; p < PlayerDice.Num(); p++)
	{
		if (PlayerDiceMatched[p]) continue;  // Already matched

		int32 PlayerVal = PlayerResults[p];
		// Combos allowed! Always consider modifiers

		// Check for direct match with any unmatched enemy dice
		for (int32 e = 0; e < EnemyDice.Num(); e++)
		{
			if (EnemyDiceMatched[e]) continue;
			if (EnemyResults[e] == PlayerVal)
			{
				return true;  // Direct match possible
			}
		}

		// Check if any modifier could create a match (combos allowed!)
		{
			for (ADiceModifier* Mod : AvailableModifiers)
			{
				// Skip RerollOne and RerollAll for now - they're wildcards (always could potentially help)
				if (Mod->ModifierType == EModifierType::RerollOne || Mod->ModifierType == EModifierType::RerollAll)
				{
					return true;  // Reroll exists = could potentially match
				}

				int32 ModifiedVal = Mod->ApplyToValue(PlayerVal);
				if (ModifiedVal == PlayerVal) continue;  // No effect

				// Check if modified value matches any enemy
				for (int32 e = 0; e < EnemyDice.Num(); e++)
				{
					if (EnemyDiceMatched[e]) continue;
					if (EnemyResults[e] == ModifiedVal)
					{
						return true;  // Match possible with modifier
					}
				}
			}
		}
	}

	return false;  // No possible matches
}

void ADiceGameManager::GiveUpRound()
{
	// Deactivate modifiers and reset camera
	DeactivateModifiers();
	ResetCamera();

	// Clear all dice immediately
	ClearAllDice();

	EnemyResults.Empty();
	PlayerResults.Empty();
	PlayerDiceMatched.Empty();
	EnemyDiceMatched.Empty();
	PlayerDiceModified.Empty();
	PlayerDiceAtModifier.Empty();
	WaitTimer = 0.0f;
	SelectionMode = 0;
	SelectedDiceIndex = -1;

	// Trigger finger chop - the callback will continue the flow
	TriggerPlayerChop();
}

void ADiceGameManager::ContinueToNextRound()
{
	// Clear dice but keep permanent modifier state
	ClearAllDice();

	EnemyResults.Empty();
	PlayerResults.Empty();
	PlayerDiceMatched.Empty();
	EnemyDiceMatched.Empty();
	PlayerDiceModified.Empty();
	PlayerDiceAtModifier.Empty();
	WaitTimer = 0.0f;
	SelectionMode = 0;
	SelectedDiceIndex = -1;

	// Reset camera to original position
	ResetCamera();

	// Enemy throws new dice
	CurrentPhase = EGamePhase::EnemyThrowing;
	EnemyThrowDice();
}

void ADiceGameManager::ResetModifiersForNewRound()
{
	// Reset all non-permanently-removed modifiers to usable state
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && !PermanentlyRemovedModifiers.Contains(Mod))
		{
			Mod->bIsUsed = false;
		}
	}
}

void ADiceGameManager::StartModifierShuffle()
{
	// Get list of available (non-permanently-removed) modifiers
	TArray<ADiceModifier*> AvailableModifiers;
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && !PermanentlyRemovedModifiers.Contains(Mod))
		{
			AvailableModifiers.Add(Mod);
		}
	}

	// If no modifiers left, skip shuffle
	if (AvailableModifiers.Num() == 0)
	{
		bModifierShuffling = false;
		ActivateModifiers();
		return;
	}

	// Reset all available modifiers to usable
	ResetModifiersForNewRound();

	// Pick a random modifier to permanently remove (if this isn't round 1)
	if (CurrentRound > 1 && AvailableModifiers.Num() > 0)
	{
		int32 RemoveIndex = FMath::RandRange(0, AvailableModifiers.Num() - 1);
		FadingModifier = AvailableModifiers[RemoveIndex];
		PermanentlyRemovedModifiers.Add(FadingModifier);
		AvailableModifiers.RemoveAt(RemoveIndex);
	}
	else
	{
		FadingModifier = nullptr;
	}

	// Store current positions and calculate target positions (shuffle!)
	ModifierStartPositions.Empty();
	ModifierTargetPositions.Empty();

	// Get all current positions
	TArray<FVector> AllPositions;
	for (ADiceModifier* Mod : AvailableModifiers)
	{
		AllPositions.Add(Mod->GetActorLocation());
	}

	// Shuffle positions for juicy effect
	for (int32 i = AllPositions.Num() - 1; i > 0; i--)
	{
		int32 j = FMath::RandRange(0, i);
		AllPositions.Swap(i, j);
	}

	// Assign shuffled positions
	for (int32 i = 0; i < AvailableModifiers.Num(); i++)
	{
		ModifierStartPositions.Add(AvailableModifiers[i]->GetActorLocation());
		ModifierTargetPositions.Add(AllPositions[i]);
	}

	bModifierShuffling = true;
	ModifierShuffleProgress = 0.0f;
	FadingModifierAlpha = 1.0f;
}

void ADiceGameManager::UpdateModifierShuffle(float DeltaTime)
{
	if (!bModifierShuffling) return;

	ModifierShuffleProgress += DeltaTime * 1.5f;  // Speed of shuffle
	float Alpha = FMath::Clamp(ModifierShuffleProgress, 0.0f, 1.0f);

	// Juicy ease
	float SmoothAlpha = EaseOutElastic(Alpha);
	float LinearAlpha = EaseOutCubic(Alpha);

	// Get available modifiers (same order as positions)
	TArray<ADiceModifier*> AvailableModifiers;
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && !PermanentlyRemovedModifiers.Contains(Mod))
		{
			AvailableModifiers.Add(Mod);
		}
	}

	// Animate modifiers to new positions
	for (int32 i = 0; i < AvailableModifiers.Num(); i++)
	{
		if (ModifierStartPositions.IsValidIndex(i) && ModifierTargetPositions.IsValidIndex(i))
		{
			ADiceModifier* Mod = AvailableModifiers[i];
			if (Mod)
			{
				// Arc movement - rise up then down
				FVector NewPos = FMath::Lerp(ModifierStartPositions[i], ModifierTargetPositions[i], LinearAlpha);
				float ArcHeight = FMath::Sin(LinearAlpha * PI) * 30.0f;
				NewPos.Z += ArcHeight;
				Mod->SetActorLocation(NewPos);
			}
		}
	}

	// Fade out the removed modifier
	if (FadingModifier)
	{
		FadingModifierAlpha = 1.0f - LinearAlpha;

		// Scale down and fade
		float FadeScale = FMath::Max(0.1f, FadingModifierAlpha);
		FadingModifier->ModifierText->SetWorldScale3D(FVector(FadeScale));

		// Shrink and rise
		FVector FadePos = FadingModifier->GetActorLocation();
		FadePos.Z += DeltaTime * 50.0f * (1.0f - FadingModifierAlpha);  // Rise as fading
		FadingModifier->SetActorLocation(FadePos);

		// Fade color
		uint8 FadeAlpha = FMath::Clamp(int32(FadingModifierAlpha * 255), 0, 255);
		FadingModifier->ModifierText->SetTextRenderColor(FColor(255, 100, 100, FadeAlpha));
	}

	// When shuffle is done
	if (ModifierShuffleProgress >= 1.0f)
	{
		// Finalize positions and update base positions for hover
		for (int32 i = 0; i < AvailableModifiers.Num(); i++)
		{
			if (ModifierTargetPositions.IsValidIndex(i))
			{
				AvailableModifiers[i]->SetActorLocation(ModifierTargetPositions[i]);
				AvailableModifiers[i]->UpdateBasePosition();
			}
		}

		// Hide the faded modifier completely
		if (FadingModifier)
		{
			FadingModifier->ModifierText->SetVisibility(false);
			FadingModifier->SetActive(false);
			FadingModifier = nullptr;
		}

		bModifierShuffling = false;
		ActivateModifiers();
	}
}

void ADiceGameManager::UpdateFaceRotationMode()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// Spawn test dice if needed
	if (!TestDice)
	{
		SpawnTestDice();
	}

	// Track key states for manual "just pressed" detection
	static bool bUpHeld = false, bDownHeld = false, bLeftHeld = false, bRightHeld = false;
	static bool bQHeld = false, bEHeld = false, bRHeld = false, bPHeld = false;
	static bool b1Held = false, b2Held = false, b3Held = false, b4Held = false, b5Held = false, b6Held = false;

	// 1-6 to select face
	if (PC->IsInputKeyDown(EKeys::One) && !b1Held) { CurrentFaceEdit = 1; b1Held = true; }
	else if (!PC->IsInputKeyDown(EKeys::One)) b1Held = false;

	if (PC->IsInputKeyDown(EKeys::Two) && !b2Held) { CurrentFaceEdit = 2; b2Held = true; }
	else if (!PC->IsInputKeyDown(EKeys::Two)) b2Held = false;

	if (PC->IsInputKeyDown(EKeys::Three) && !b3Held) { CurrentFaceEdit = 3; b3Held = true; }
	else if (!PC->IsInputKeyDown(EKeys::Three)) b3Held = false;

	if (PC->IsInputKeyDown(EKeys::Four) && !b4Held) { CurrentFaceEdit = 4; b4Held = true; }
	else if (!PC->IsInputKeyDown(EKeys::Four)) b4Held = false;

	if (PC->IsInputKeyDown(EKeys::Five) && !b5Held) { CurrentFaceEdit = 5; b5Held = true; }
	else if (!PC->IsInputKeyDown(EKeys::Five)) b5Held = false;

	if (PC->IsInputKeyDown(EKeys::Six) && !b6Held) { CurrentFaceEdit = 6; b6Held = true; }
	else if (!PC->IsInputKeyDown(EKeys::Six)) b6Held = false;

	// Adjust step
	float Step = 15.0f;
	if (PC->IsInputKeyDown(EKeys::LeftShift)) Step = 45.0f;

	FRotator& Rot = FaceRotations[CurrentFaceEdit];

	// Arrow keys for Pitch/Yaw
	if (PC->IsInputKeyDown(EKeys::Up) && !bUpHeld) { Rot.Pitch += Step; bUpHeld = true; }
	else if (!PC->IsInputKeyDown(EKeys::Up)) bUpHeld = false;

	if (PC->IsInputKeyDown(EKeys::Down) && !bDownHeld) { Rot.Pitch -= Step; bDownHeld = true; }
	else if (!PC->IsInputKeyDown(EKeys::Down)) bDownHeld = false;

	if (PC->IsInputKeyDown(EKeys::Left) && !bLeftHeld) { Rot.Yaw -= Step; bLeftHeld = true; }
	else if (!PC->IsInputKeyDown(EKeys::Left)) bLeftHeld = false;

	if (PC->IsInputKeyDown(EKeys::Right) && !bRightHeld) { Rot.Yaw += Step; bRightHeld = true; }
	else if (!PC->IsInputKeyDown(EKeys::Right)) bRightHeld = false;

	// Q/E for Roll
	if (PC->IsInputKeyDown(EKeys::Q) && !bQHeld) { Rot.Roll -= Step; bQHeld = true; }
	else if (!PC->IsInputKeyDown(EKeys::Q)) bQHeld = false;

	if (PC->IsInputKeyDown(EKeys::E) && !bEHeld) { Rot.Roll += Step; bEHeld = true; }
	else if (!PC->IsInputKeyDown(EKeys::E)) bEHeld = false;

	// R to reset current face
	if (PC->IsInputKeyDown(EKeys::R) && !bRHeld) { Rot = FRotator::ZeroRotator; bRHeld = true; }
	else if (!PC->IsInputKeyDown(EKeys::R)) bRHeld = false;

	// P to print all rotations
	if (PC->IsInputKeyDown(EKeys::P) && !bPHeld)
	{
		bPHeld = true;
		UE_LOG(LogTemp, Warning, TEXT("=== FACE ROTATIONS ==="));
		for (int32 i = 1; i <= 6; i++)
		{
			UE_LOG(LogTemp, Warning, TEXT("FaceRotations[%d] = FRotator(%.0f, %.0f, %.0f);"),
				i, FaceRotations[i].Pitch, FaceRotations[i].Yaw, FaceRotations[i].Roll);
		}
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Printed to Output Log!"));
	}
	else if (!PC->IsInputKeyDown(EKeys::P)) bPHeld = false;

	UpdateTestDice();

	// Draw HUD
	GEngine->AddOnScreenDebugMessage(200, 0.0f, FColor::Cyan, TEXT("=== FACE ROTATION TOOL (F to exit) ==="));
	GEngine->AddOnScreenDebugMessage(201, 0.0f, FColor::White, TEXT("1-6: Select face | Arrows: Pitch/Yaw | Q/E: Roll | P: Print"));

	for (int32 i = 1; i <= 6; i++)
	{
		FColor Col = (i == CurrentFaceEdit) ? FColor::Yellow : FColor::Silver;
		FString Marker = (i == CurrentFaceEdit) ? TEXT(">> ") : TEXT("   ");
		GEngine->AddOnScreenDebugMessage(202 + i, 0.0f, Col,
			FString::Printf(TEXT("%sFace %d: (%.0f, %.0f, %.0f)"),
				*Marker, i, FaceRotations[i].Pitch, FaceRotations[i].Yaw, FaceRotations[i].Roll));
	}
}

// ===== HAND INTEGRATION =====

UPlayerHandComponent* ADiceGameManager::GetPlayerHand()
{
	if (PlayerHandActor)
	{
		return PlayerHandActor->FindComponentByClass<UPlayerHandComponent>();
	}
	return nullptr;
}

UPlayerHandComponent* ADiceGameManager::GetEnemyHand()
{
	if (EnemyHandActor)
	{
		return EnemyHandActor->FindComponentByClass<UPlayerHandComponent>();
	}
	return nullptr;
}

void ADiceGameManager::TriggerPlayerChop()
{
	UPlayerHandComponent* Hand = GetPlayerHand();
	if (Hand && Hand->FingersRemaining > 0)
	{
		bWaitingForChop = true;
		Hand->ChopNextFinger();
	}
	else
	{
		// No hand or no fingers left, just continue
		OnPlayerChopComplete();
	}
}

void ADiceGameManager::TriggerEnemyChop()
{
	UPlayerHandComponent* Hand = GetEnemyHand();
	if (Hand && Hand->FingersRemaining > 0)
	{
		bWaitingForChop = true;
		Hand->ChopNextFinger();
	}
	else
	{
		// No hand or no fingers left, just continue
		OnEnemyChopComplete();
	}
}

void ADiceGameManager::OnPlayerChopComplete()
{
	bWaitingForChop = false;

	// Check for game over (no fingers left = dead)
	UPlayerHandComponent* Hand = GetPlayerHand();
	if (Hand && Hand->FingersRemaining <= 0)
	{
		CurrentPhase = EGamePhase::GameOver;
		return;
	}

	// Continue to next round - enemy throws automatically
	CurrentRound++;
	ClearAllDice();

	EnemyResults.Empty();
	PlayerResults.Empty();
	PlayerDiceMatched.Empty();
	EnemyDiceMatched.Empty();
	PlayerDiceModified.Empty();
	PlayerDiceAtModifier.Empty();
	WaitTimer = 0.0f;
	SelectionMode = 0;
	SelectedDiceIndex = -1;

	// Enemy throws new dice
	CurrentPhase = EGamePhase::EnemyThrowing;
	EnemyThrowDice();
}

void ADiceGameManager::OnEnemyChopComplete()
{
	bWaitingForChop = false;

	// Check for game over
	UPlayerHandComponent* Hand = GetEnemyHand();
	if (Hand && Hand->FingersRemaining <= 0)
	{
		CurrentPhase = EGamePhase::GameOver;
		return;
	}

	// Player won the round, continue to next round - enemy throws again
	CurrentRound++;

	CurrentPhase = EGamePhase::EnemyThrowing;
	EnemyThrowDice();
}
