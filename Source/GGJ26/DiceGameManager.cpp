#include "DiceGameManager.h"
#include "MaskEnemy.h"
#include "DiceCamera.h"
#include "PlayerHandComponent.h"
#include "RoundTimerComponent.h"
#include "SoundManager.h"
#include "HangingBoardComponent.h"
#include "IRButtonComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Components/InputComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"

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
	LastHoveredDice = nullptr;

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

	// Round timer
	RoundTimerActor = nullptr;
	CachedRoundTimer = nullptr;

	// Sound
	SoundManager = nullptr;

	// Bonus round
	HangingBoardActor = nullptr;
	YesButtonActor = nullptr;
	NoButtonActor = nullptr;
	bEnableBonusRound = true;
	CachedHangingBoard = nullptr;
	CachedYesButton = nullptr;
	CachedNoButton = nullptr;
	bWaitingForBonusChoice = false;
	bBonusRoundAccepted = false;

	// Bonus round camera
	BonusCameraDistance = 80.0f;   // Back from buttons (X)
	BonusCameraSide = 0.0f;        // Left/right (Y)
	BonusCameraHeight = 50.0f;     // Above buttons (Z)
	BonusCameraPitch = 0.0f;       // 0 = auto look at buttons, or override
	BonusCameraFocusSpeed = 2.5f;
	bDebugBonusCamera = false;

	// Masquerade UI
	MasqueradeUIActor = nullptr;
	MasqueradeUIText = nullptr;
	TypewriterSpeed = 15.0f;  // Characters per second
	GlitchChance = 0.3f;      // 30% chance of glitch per character
	TypewriterTimer = 0.0f;
	TypewriterIndex = 0;
	bTypewriterActive = false;
	bTypewriterOut = false;
	GlitchTimer = 0.0f;
	bShowingGlitch = false;
	GlitchChars = TEXT("@#$%&*!?<>[]{}|/\\");

	// Dice Label (fold prompt)
	DiceLabelActor = nullptr;
	DiceLabelTextComp = nullptr;
	DiceLabelText = TEXT("SPACE TO FOLD");
	DiceLabelTypeSpeed = 20.0f;
	DiceLabelFullText = TEXT("");
	DiceLabelCurrentText = TEXT("");
	DiceLabelTypeTimer = 0.0f;
	DiceLabelTypeIndex = 0;
	bDiceLabelTypewriterActive = false;
	bDiceLabelTypewriterOut = false;

	// Masked dice
	MaskedDiceMesh = nullptr;
	MaskedDiceMaterial = nullptr;
	BonusDiceSpawnPos = FVector(0.0f, 0.0f, 100.0f);
	BonusPlayerDicePos = FVector(0.0f, 50.0f, 10.0f);
	BonusDiceSpacing = 30.0f;

	// Bonus round gameplay
	BonusPhase = 0;  // Inactive
	BonusPlayerDice = nullptr;
	BonusRevealDice = nullptr;
	BonusEnemyTotal = 0;
	bBonusIsHigher = false;
	bPlayerGuessedHigher = false;
	BonusDiceModifier = 0;
	bBonusWon = false;
	bBonusActiveThisRound = false;
	bBonusRoundJustEnded = false;
	BonusAnimTimer = 0.0f;
	BonusLineupProgress = 0.0f;
	BonusPlayerLineupProgress = 0.0f;
	BonusRevealProgress = 0.0f;
	BonusCameraProgress = 0.0f;
	bBonusCameraFocusing = false;
	bBonusCameraReturning = false;

	// Bonus dice snap animation
	bBonusDiceSnapping = false;
	BonusSnapProgress = 0.0f;
	bBonusSnapChoseHigher = false;
	SelectedBonusModifier = nullptr;

	// Bonus reveal animation
	BonusRevealPhase = 0;
	RevealShakeTimer = 0.0f;
	RevealStrikeProgress = 0.0f;
	BonusPlayerVelocity = FVector::ZeroVector;
	BonusRevealVelocity = FVector::ZeroVector;
	BonusPlayerRotSpeed = 0.0f;
	BonusRevealRotSpeed = 0.0f;
	BonusBounceCount = 0;
	BonusResultFloorZ = 0.0f;

	// Bonus camera shake
	bBonusCameraShaking = false;
	BonusCameraShakeTimer = 0.0f;
	BonusCameraShakeDuration = 0.0f;
	BonusCameraShakeIntensity = 0.0f;
	BonusCameraShakeOffset = FVector::ZeroVector;

	// Win sequence
	WinSequencePhase = 0;
	WinSequenceTimer = 0.0f;
	WinSequenceProgress = 0.0f;
	WinMaskRotationProgress = 0.0f;
	WinFadeAlpha = 0.0f;
	FadeWidgetClass = nullptr;

	// Win camera breathing
	bWinCameraBreathing = false;
	WinCameraBreathTimer = 0.0f;
	WinCameraBasePos = FVector::ZeroVector;
	WinCameraBaseRot = FRotator::ZeroRotator;

	// Mask mesh animation
	WinMaskMeshStartPos = FVector::ZeroVector;
	WinMaskMeshStartRot = FRotator::ZeroRotator;
	WinMaskMeshTargetPos = FVector::ZeroVector;
	WinMaskMeshTargetRot = FRotator::ZeroRotator;
	FadeWidgetInstance = nullptr;
	BlackImageWidget = nullptr;

	// Lose sequence
	PlayerMaskMesh = nullptr;
	PlayerMaskMaterial = nullptr;
	PlayerMaskDropScale = 0.1f;
	PlayerMaskDropRotation = FRotator(-90.0f, 0.0f, 0.0f);  // Facing down by default
	LoseCameraForwardOffset = 50.0f;  // Move forward a bit by default
	LoseSequencePhase = 0;
	LoseSequenceTimer = 0.0f;
	LoseSequenceProgress = 0.0f;
	LoseFadeAlpha = 0.0f;
	LoseCameraStartPos = FVector::ZeroVector;
	LoseCameraStartRot = FRotator::ZeroRotator;
	LoseCameraTargetRot = FRotator::ZeroRotator;
	bLoseCameraBreathing = false;
	LoseCameraBreathTimer = 0.0f;
	DroppedPlayerMask = nullptr;
	PlayerMaskDropStartPos = FVector::ZeroVector;
	DroppedKnife = nullptr;
}

void ADiceGameManager::BeginPlay()
{
	Super::BeginPlay();
	SetupInputBindings();
	FindAllModifiers();

	// Cache and hide the dice label at start
	if (DiceLabelActor)
	{
		DiceLabelTextComp = DiceLabelActor->FindComponentByClass<UTextRenderComponent>();
		if (DiceLabelTextComp)
		{
			DiceLabelTextComp->SetText(FText::FromString(TEXT("")));
		}
		DiceLabelActor->SetActorHiddenInGame(true);
	}

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

	// Bind to round timer expired event
	URoundTimerComponent* Timer = GetRoundTimer();
	if (Timer)
	{
		Timer->OnTimerExpired.AddDynamic(this, &ADiceGameManager::OnRoundTimerExpired);
	}

	// Bind to bonus round events
	UHangingBoardComponent* Board = GetHangingBoard();
	if (Board)
	{
		Board->OnBoardArrived.AddDynamic(this, &ADiceGameManager::OnBoardArrived);
		Board->OnBoardRetracted.AddDynamic(this, &ADiceGameManager::OnBoardRetracted);
	}
	UIRButtonComponent* YesBtn = GetYesButton();
	if (YesBtn)
	{
		YesBtn->OnButtonPressed.AddDynamic(this, &ADiceGameManager::OnBonusButtonPressed);
	}
	UIRButtonComponent* NoBtn = GetNoButton();
	if (NoBtn)
	{
		NoBtn->OnButtonPressed.AddDynamic(this, &ADiceGameManager::OnBonusButtonPressed);
	}

	// Hide Masquerade UI by default
	if (MasqueradeUIActor)
	{
		MasqueradeUIActor->SetActorHiddenInGame(true);
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
			InputComponent->BindKey(EKeys::X, IE_Pressed, this, &ADiceGameManager::OnDebugKillEnemy);  // Debug damage enemy
			InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ADiceGameManager::OnDebugKillPlayer); // Debug damage player
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
			// Bonus modifiers start hidden (only visible during bonus round)
			if (Mod->ModifierType == EModifierType::BonusHigher ||
				Mod->ModifierType == EModifierType::BonusLower)
			{
				Mod->SetActive(false);
				Mod->SetHidden(true);  // Completely hidden
			}
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

	// Update bonus round camera
	UpdateBonusCameraFocus(DeltaTime);

	// Update bonus round gameplay
	UpdateBonusRound(DeltaTime);

	// Update bonus dice snap animation
	UpdateBonusDiceSnap(DeltaTime);

	// Update bonus camera shake
	UpdateBonusCameraShake(DeltaTime);

	// Update masquerade UI typewriter effect
	UpdateMasqueradeTypewriter(DeltaTime);

	// Update dice label typewriter effect
	UpdateDiceLabelTypewriter(DeltaTime);

	// Update win sequence
	UpdateWinSequence(DeltaTime);

	// Update lose sequence
	UpdateLoseSequence(DeltaTime);

	// Debug: lock camera to bonus button view
	if (bDebugBonusCamera)
	{
		// Calculate bonus camera position
		FVector ButtonCenter = FVector::ZeroVector;
		int32 ButtonCount = 0;
		if (YesButtonActor) { ButtonCenter += YesButtonActor->GetActorLocation(); ButtonCount++; }
		if (NoButtonActor) { ButtonCenter += NoButtonActor->GetActorLocation(); ButtonCount++; }
		if (ButtonCount > 0) ButtonCenter /= ButtonCount;

		FVector DebugCamPos;
		DebugCamPos.X = ButtonCenter.X - BonusCameraDistance;
		DebugCamPos.Y = ButtonCenter.Y + BonusCameraSide;
		DebugCamPos.Z = ButtonCenter.Z + BonusCameraHeight;

		FRotator DebugCamRot;
		if (FMath::IsNearlyZero(BonusCameraPitch))
		{
			// Auto look at buttons
			FVector LookDir = (ButtonCenter - DebugCamPos).GetSafeNormal();
			DebugCamRot = LookDir.Rotation();
		}
		else
		{
			// Use manual pitch, auto yaw
			FVector LookDir = (ButtonCenter - DebugCamPos).GetSafeNormal();
			DebugCamRot = FRotator(BonusCameraPitch, LookDir.Rotation().Yaw, 0.0f);
		}

		ADiceCamera* Cam = FindCamera();
		if (Cam)
		{
			Cam->SetActorLocation(DebugCamPos);
			Cam->SetActorRotation(DebugCamRot);
		}
	}

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
		// Continue to next round
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
				// Keep bonus modifiers hidden - they only show during bonus round
				if (Mod->ModifierType != EModifierType::BonusHigher &&
					Mod->ModifierType != EModifierType::BonusLower)
				{
					Mod->ModifierText->SetVisibility(true);
				}
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
				// Keep bonus modifiers hidden - they only show during bonus round
				if (Mod->ModifierType != EModifierType::BonusHigher &&
					Mod->ModifierType != EModifierType::BonusLower)
				{
					Mod->ModifierText->SetVisibility(true);
				}
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

void ADiceGameManager::OnDebugKillEnemy()
{
	// Debug - reduce enemy health by 1
	EnemyHealth = FMath::Max(0, EnemyHealth - 1);
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("DEBUG: Enemy HP = %d"), EnemyHealth));
}

void ADiceGameManager::OnDebugKillPlayer()
{
	// Debug - reduce player health by 1
	PlayerHealth = FMath::Max(0, PlayerHealth - 1);
	GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("DEBUG: Player HP = %d"), PlayerHealth));
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
	// Hide the fold prompt at game start
	HideDiceLabel();

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
			// Make sure regular modifiers are visible (keep bonus modifiers hidden)
			if (Mod->ModifierType != EModifierType::BonusHigher &&
				Mod->ModifierType != EModifierType::BonusLower)
			{
				Mod->ModifierText->SetVisibility(true);
			}
			else
			{
				Mod->SetHidden(true);
			}
		}
	}

	// Ensure camera is at original position
	ResetCamera();

	CurrentPhase = EGamePhase::EnemyThrowing;
	EnemyThrowDice();
}

void ADiceGameManager::EnemyThrowDice()
{
	// Set timer to DEALING state
	URoundTimerComponent* Timer = GetRoundTimer();
	if (Timer)
	{
		Timer->SetDealing();
	}

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
		if (D)
		{
			if (D->IsStill())
			{
				// Play sound for this dice if it just landed
				if (!D->bHasPlayedLandSound)
				{
					D->bHasPlayedLandSound = true;
					if (SoundManager)
					{
						SoundManager->PlayDiceRollAtLocation(D->GetActorLocation());
					}
				}
			}
			else
			{
				AllSettled = false;
			}
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

	// Set timer to show "E TO BLITZ"
	URoundTimerComponent* Timer = GetRoundTimer();
	if (Timer)
	{
		Timer->SetPlayerReady();
	}
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

		if (D->IsStill())
		{
			// Play sound for this dice if it just landed
			if (!D->bHasPlayedLandSound)
			{
				D->bHasPlayedLandSound = true;
				if (SoundManager)
				{
					SoundManager->PlayDiceRollAtLocation(D->GetActorLocation());
				}
			}
		}
		else
		{
			AllSettled = false;
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
	UE_LOG(LogTemp, Warning, TEXT("StartMatchingPhase - CurrentRound: %d"), CurrentRound);

	CurrentPhase = EGamePhase::PlayerMatching;
	SelectionMode = 0;
	SelectedDiceIndex = 0;
	LastHoveredDice = nullptr;

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

	// Show the fold prompt with typewriter effect
	StartDiceLabelTypewriter();
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
	else
	{
		// Play error sound for invalid match
		if (SoundManager)
		{
			SoundManager->PlayError();
		}
	}
}

void ADiceGameManager::StartMatchAnimation(int32 PlayerIdx, int32 EnemyIdx)
{
	if (!PlayerDice.IsValidIndex(PlayerIdx) || !EnemyDice.IsValidIndex(EnemyIdx)) return;

	// Play match sound
	if (SoundManager)
	{
		SoundManager->PlayDiceMatch();
	}

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

	// Check if modifier would have no effect or result in invalid value
	if (Modifier->ModifierType == EModifierType::MinusOne && OldValue <= 1)
	{
		// Can't go below 1 - reject
		if (SoundManager) SoundManager->PlayError();
		return;
	}
	if (Modifier->ModifierType == EModifierType::PlusOne && OldValue >= 6)
	{
		// Can't go above 6 - reject
		if (SoundManager) SoundManager->PlayError();
		return;
	}
	if (Modifier->ModifierType == EModifierType::PlusTwo && OldValue >= 5)
	{
		// Can't go above 6 - reject (5+2=7, 6+2=8 are invalid)
		if (SoundManager) SoundManager->PlayError();
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

	// Play dice roll sound immediately on reroll
	if (SoundManager)
	{
		SoundManager->PlayDiceRollAtLocation(Dice->GetActorLocation());
	}

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

	// Skip landing sound since we already played on throw
	Dice->bHasPlayedLandSound = true;

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

		// Play dice roll sound immediately for each dice
		if (SoundManager)
		{
			SoundManager->PlayDiceRollAtLocation(Dice->GetActorLocation());
		}

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

		// Skip landing sound since we already played on throw
		Dice->bHasPlayedLandSound = true;

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
		// Hide the fold prompt
		StartDiceLabelTypewriterOut();

		// Stop the timer - player won this round!
		URoundTimerComponent* Timer = GetRoundTimer();
		if (Timer)
		{
			Timer->StopCountdown();
			Timer->SetHold();
		}

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
	if (EnemyHealth <= 0)
	{
		// Player wins! Start win sequence
		CurrentPhase = EGamePhase::GameOver;
		StartWinSequence();
	}
	else if (PlayerHealth <= 0)
	{
		// Player loses - start lose sequence
		CurrentPhase = EGamePhase::GameOver;
		StartLoseSequence();
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
	// Allow during bonus round phase 7 (player turn) but not during snap
	if (BonusPhase == 7)
	{
		if (bBonusDiceSnapping) return;  // Block during snap animation

		FVector HitLocation;
		AActor* HitActor = GetActorUnderMouse(HitLocation);
		if (HitActor && HitActor == BonusPlayerDice)
		{
			StartDragging(BonusPlayerDice, -99);  // Special index for bonus dice
			return;
		}
		return;
	}

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
	// Handle bonus round dice release
	if (BonusPhase == 7 && bIsDragging && DraggedDice == BonusPlayerDice)
	{
		FVector DicePos = DraggedDice->GetActorLocation();

		// Check for HIGHER/LOWER modifiers
		float ClosestDist = 60.0f;
		ADiceModifier* ClosestMod = nullptr;

		for (ADiceModifier* Mod : AllModifiers)
		{
			if (Mod && !Mod->bIsUsed && Mod->bIsActive)
			{
				if (Mod->ModifierType == EModifierType::BonusHigher ||
					Mod->ModifierType == EModifierType::BonusLower)
				{
					float Dist = FVector::Dist2D(DicePos, Mod->GetActorLocation());
					if (Dist < ClosestDist)
					{
						ClosestDist = Dist;
						ClosestMod = Mod;
					}
				}
			}
		}

		if (ClosestMod)
		{
			// Player made a choice!
			bool bChoseHigher = (ClosestMod->ModifierType == EModifierType::BonusHigher);
			ClosestMod->UseModifier();

			// Stop dragging state
			DraggedDice->SetHighlighted(false);
			DraggedDice->bIsBeingDragged = false;
			bIsDragging = false;
			DraggedDice = nullptr;
			DraggedDiceIndex = -1;

			// Start snap animation instead of immediately processing
			StartBonusDiceSnap(ClosestMod, bChoseHigher);
		}
		else
		{
			// No modifier hit - bounce back
			PhysicsBounceBack();
		}
		return;
	}

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

		// Play hover sound when hovering a new dice
		if (NewHoveredDice != nullptr && NewHoveredDice != LastHoveredDice)
		{
			if (SoundManager)
			{
				SoundManager->PlayHover();
			}
		}
		LastHoveredDice = NewHoveredDice;

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
	// Play pickup sound
	if (SoundManager)
	{
		SoundManager->PlayPickUp();
	}

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
	if (CurrentPhase == EGamePhase::GameOver) return;  // Don't highlight during win/lose
	if (!PlayerResults.IsValidIndex(DraggedDiceIndex)) return;  // Bounds check

	int32 DraggedValue = PlayerResults[DraggedDiceIndex];

	// Highlight enemy dice with matching values
	for (int32 i = 0; i < EnemyDice.Num(); i++)
	{
		if (EnemyDice[i] && EnemyDiceMatched.IsValidIndex(i) && !EnemyDiceMatched[i] && EnemyResults.IsValidIndex(i))
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
		// Skip permanently removed modifiers and bonus modifiers
		if (Mod && !Mod->bIsUsed && !PermanentlyRemovedModifiers.Contains(Mod))
		{
			// Bonus modifiers only activate during bonus round
			if (Mod->ModifierType == EModifierType::BonusHigher ||
				Mod->ModifierType == EModifierType::BonusLower)
			{
				continue;
			}
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
	// Hide the fold prompt
	StartDiceLabelTypewriterOut();

	// Stop the timer
	URoundTimerComponent* Timer = GetRoundTimer();
	if (Timer)
	{
		Timer->StopCountdown();
		Timer->SetHold();
	}

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
	// Increment round counter
	CurrentRound++;
	UE_LOG(LogTemp, Warning, TEXT("ContinueToNextRound - Now Round %d"), CurrentRound);

	// Handle temporary bonus dice reset logic:
	// - If we just finished a bonus round, don't reset yet (play this round with bonus dice)
	// - If we're finishing the bonus-affected round, reset back to base dice count
	if (bBonusRoundJustEnded)
	{
		// Just came from bonus round - keep the modified dice count for THIS round
		// Next round will reset
		bBonusRoundJustEnded = false;
		// bBonusActiveThisRound was set in ShowBonusResult, so next call will reset
		UE_LOG(LogTemp, Warning, TEXT("Starting bonus-affected round with %d dice"), PlayerNumDice);
	}
	else if (bBonusActiveThisRound)
	{
		// Finished the bonus-affected round - reset dice count to base
		PlayerNumDice = BaseDiceCount;
		bBonusActiveThisRound = false;
		UE_LOG(LogTemp, Warning, TEXT("Bonus effect expired, reset to %d dice"), PlayerNumDice);
	}

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
	// Get list of available (non-permanently-removed) modifiers - exclude bonus modifiers
	TArray<ADiceModifier*> AvailableModifiers;
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && !PermanentlyRemovedModifiers.Contains(Mod) &&
			Mod->ModifierType != EModifierType::BonusHigher &&
			Mod->ModifierType != EModifierType::BonusLower)
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
	if (CurrentRound > 1 && AvailableModifiers.Num() > 1)  // Keep at least 1 modifier
	{
		int32 RemoveIndex = FMath::RandRange(0, AvailableModifiers.Num() - 1);
		FadingModifier = AvailableModifiers[RemoveIndex];
		PermanentlyRemovedModifiers.Add(FadingModifier);
		AvailableModifiers.RemoveAt(RemoveIndex);
		UE_LOG(LogTemp, Warning, TEXT("Round %d: Removing modifier '%s' (%d remaining)"),
			CurrentRound, *FadingModifier->GetModifierDisplayText(), AvailableModifiers.Num());
	}
	else
	{
		FadingModifier = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Round %d: No modifier removed (%d available)"),
			CurrentRound, AvailableModifiers.Num());
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

	// Get available modifiers (same order as positions) - exclude bonus modifiers
	TArray<ADiceModifier*> AvailableModifiers;
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && !PermanentlyRemovedModifiers.Contains(Mod) &&
			Mod->ModifierType != EModifierType::BonusHigher &&
			Mod->ModifierType != EModifierType::BonusLower)
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

	// Fade out the removed modifier - simple fade, no glitchy scaling
	if (FadingModifier && FadingModifier->ModifierText)
	{
		FadingModifierAlpha = 1.0f - LinearAlpha;

		// Just fade the color smoothly
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
			FadingModifier->SetHidden(true);  // Hide entire actor
			FadingModifier->SetActive(false);
			UE_LOG(LogTemp, Warning, TEXT("Modifier '%s' permanently removed (Round %d)"),
				*FadingModifier->GetModifierDisplayText(), CurrentRound);
			FadingModifier = nullptr;
		}

		bModifierShuffling = false;
		ActivateModifiers();

		// Start the countdown timer now that matching phase is ready
		URoundTimerComponent* Timer = GetRoundTimer();
		if (Timer)
		{
			Timer->StartCountdown();
		}
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

URoundTimerComponent* ADiceGameManager::GetRoundTimer()
{
	if (CachedRoundTimer)
	{
		return CachedRoundTimer;
	}

	if (RoundTimerActor)
	{
		CachedRoundTimer = RoundTimerActor->FindComponentByClass<URoundTimerComponent>();
		return CachedRoundTimer;
	}
	return nullptr;
}

void ADiceGameManager::OnRoundTimerExpired()
{
	// Time's up! Player loses this round
	if (CurrentPhase == EGamePhase::PlayerMatching)
	{
		// Stop the timer display
		URoundTimerComponent* Timer = GetRoundTimer();
		if (Timer)
		{
			Timer->StopCountdown();
		}

		GiveUpRound();
	}
}

void ADiceGameManager::TriggerPlayerChop()
{
	// Deal damage to player health
	DealDamage(false);

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
	// Deal damage to enemy health
	DealDamage(true);

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

	// Check for game over using health system
	if (PlayerHealth <= 0)
	{
		CheckGameOver();
		return;
	}

	// Trigger bonus round prompt if enabled
	if (bEnableBonusRound)
	{
		StartBonusRoundPrompt();
	}
	else
	{
		ContinueToNextRound();
	}
}

void ADiceGameManager::OnEnemyChopComplete()
{
	bWaitingForChop = false;

	// Check for game over using health system (will trigger win sequence if enemy dead)
	if (EnemyHealth <= 0)
	{
		CheckGameOver();
		return;
	}

	// Trigger bonus round prompt if enabled
	if (bEnableBonusRound)
	{
		StartBonusRoundPrompt();
	}
	else
	{
		ContinueToNextRound();
	}
}

// ==================== BONUS ROUND ====================

UHangingBoardComponent* ADiceGameManager::GetHangingBoard()
{
	if (CachedHangingBoard)
	{
		return CachedHangingBoard;
	}

	if (HangingBoardActor)
	{
		CachedHangingBoard = HangingBoardActor->FindComponentByClass<UHangingBoardComponent>();
		return CachedHangingBoard;
	}
	return nullptr;
}

UIRButtonComponent* ADiceGameManager::GetYesButton()
{
	if (CachedYesButton)
	{
		return CachedYesButton;
	}

	if (YesButtonActor)
	{
		CachedYesButton = YesButtonActor->FindComponentByClass<UIRButtonComponent>();
		return CachedYesButton;
	}
	return nullptr;
}

UIRButtonComponent* ADiceGameManager::GetNoButton()
{
	if (CachedNoButton)
	{
		return CachedNoButton;
	}

	if (NoButtonActor)
	{
		CachedNoButton = NoButtonActor->FindComponentByClass<UIRButtonComponent>();
		return CachedNoButton;
	}
	return nullptr;
}

void ADiceGameManager::StartBonusRoundPrompt()
{
	bWaitingForBonusChoice = true;
	bBonusRoundAccepted = false;

	// Show the hanging board
	UHangingBoardComponent* Board = GetHangingBoard();
	if (Board)
	{
		Board->ShowBoard(TEXT("MASQUERADE?"));
	}
}

void ADiceGameManager::OnBoardArrived()
{
	// Board has arrived, now activate both buttons
	UIRButtonComponent* YesBtn = GetYesButton();
	if (YesBtn)
	{
		YesBtn->ActivateButton();
	}
	UIRButtonComponent* NoBtn = GetNoButton();
	if (NoBtn)
	{
		NoBtn->ActivateButton();
	}

	// Focus camera on buttons
	StartBonusButtonCameraFocus();
}

void ADiceGameManager::OnBonusButtonPressed(EIRButtonType ButtonType)
{
	// Deactivate both buttons
	UIRButtonComponent* YesBtn = GetYesButton();
	if (YesBtn)
	{
		YesBtn->DeactivateButton();
	}
	UIRButtonComponent* NoBtn = GetNoButton();
	if (NoBtn)
	{
		NoBtn->DeactivateButton();
	}

	// Hide the board
	UHangingBoardComponent* Board = GetHangingBoard();
	if (Board)
	{
		Board->HideBoard();
	}

	// This is the initial Masquerade? question
	if (ButtonType == EIRButtonType::Yes)
	{
		bBonusRoundAccepted = true;
		UE_LOG(LogTemp, Warning, TEXT("BONUS ROUND ACCEPTED! Starting Masquerade..."));
	}
	else
	{
		bBonusRoundAccepted = false;
		UE_LOG(LogTemp, Warning, TEXT("Bonus round declined. Continuing..."));
	}

	// Return camera to game view
	StartBonusButtonCameraReturn();
}

void ADiceGameManager::OnBoardRetracted()
{
	bWaitingForBonusChoice = false;

	if (bBonusRoundAccepted)
	{
		// Start the bonus round minigame!
		StartBonusRoundGame();
	}
	else
	{
		// Player chose NO, continue with normal game
		ContinueToNextRound();
	}
}

void ADiceGameManager::StartBonusButtonCameraFocus()
{
	ADiceCamera* Cam = FindCamera();
	if (!Cam) return;

	// Store original camera position (use the game's stored original, not current)
	BonusCameraOriginalPos = OriginalCameraLocation;
	BonusCameraOriginalRot = OriginalCameraRotation;

	// Find center between both buttons
	FVector ButtonCenter = FVector::ZeroVector;
	int32 ButtonCount = 0;

	if (YesButtonActor)
	{
		ButtonCenter += YesButtonActor->GetActorLocation();
		ButtonCount++;
	}
	if (NoButtonActor)
	{
		ButtonCenter += NoButtonActor->GetActorLocation();
		ButtonCount++;
	}

	if (ButtonCount > 0)
	{
		ButtonCenter /= ButtonCount;
	}

	// Position with all offsets
	BonusCameraTargetPos.X = ButtonCenter.X - BonusCameraDistance;
	BonusCameraTargetPos.Y = ButtonCenter.Y + BonusCameraSide;
	BonusCameraTargetPos.Z = ButtonCenter.Z + BonusCameraHeight;

	// Look at button center (use manual pitch if set)
	FVector LookDir = (ButtonCenter - BonusCameraTargetPos).GetSafeNormal();
	if (FMath::IsNearlyZero(BonusCameraPitch))
	{
		BonusCameraTargetRot = LookDir.Rotation();
	}
	else
	{
		BonusCameraTargetRot = FRotator(BonusCameraPitch, LookDir.Rotation().Yaw, 0.0f);
	}

	UE_LOG(LogTemp, Warning, TEXT("Bonus Camera: Target Pos (%.1f, %.1f, %.1f), Button Center (%.1f, %.1f, %.1f)"),
		BonusCameraTargetPos.X, BonusCameraTargetPos.Y, BonusCameraTargetPos.Z,
		ButtonCenter.X, ButtonCenter.Y, ButtonCenter.Z);

	BonusCameraProgress = 0.0f;
	bBonusCameraFocusing = true;
	bBonusCameraReturning = false;
}

void ADiceGameManager::StartBonusButtonCameraReturn()
{
	BonusCameraProgress = 0.0f;
	bBonusCameraFocusing = false;
	bBonusCameraReturning = true;
}

void ADiceGameManager::UpdateBonusCameraFocus(float DeltaTime)
{
	ADiceCamera* Cam = FindCamera();
	if (!Cam) return;

	if (bBonusCameraFocusing)
	{
		BonusCameraProgress += DeltaTime * BonusCameraFocusSpeed;
		BonusCameraProgress = FMath::Clamp(BonusCameraProgress, 0.0f, 1.0f);

		float T = EaseOutCubic(BonusCameraProgress);
		FVector NewPos = FMath::Lerp(BonusCameraOriginalPos, BonusCameraTargetPos, T);
		FRotator NewRot = FMath::Lerp(BonusCameraOriginalRot, BonusCameraTargetRot, T);
		Cam->SetActorLocation(NewPos);
		Cam->SetActorRotation(NewRot);

		if (BonusCameraProgress >= 1.0f)
		{
			bBonusCameraFocusing = false;
		}
	}
	else if (bBonusCameraReturning)
	{
		BonusCameraProgress += DeltaTime * BonusCameraFocusSpeed;
		BonusCameraProgress = FMath::Clamp(BonusCameraProgress, 0.0f, 1.0f);

		float T = EaseOutCubic(BonusCameraProgress);
		FVector NewPos = FMath::Lerp(BonusCameraTargetPos, BonusCameraOriginalPos, T);
		FRotator NewRot = FMath::Lerp(BonusCameraTargetRot, BonusCameraOriginalRot, T);
		Cam->SetActorLocation(NewPos);
		Cam->SetActorRotation(NewRot);

		if (BonusCameraProgress >= 1.0f)
		{
			bBonusCameraReturning = false;
		}
	}
}

// ==================== BONUS ROUND GAMEPLAY ====================

int32 ADiceGameManager::GenerateBonusTotal()
{
	// Generate two dice values where sum is NOT 7
	int32 Die1, Die2, Total;
	do
	{
		Die1 = FMath::RandRange(1, 6);
		Die2 = FMath::RandRange(1, 6);
		Total = Die1 + Die2;
	} while (Total == 7);

	return Total;
}

void ADiceGameManager::StartBonusRoundGame()
{
	UE_LOG(LogTemp, Warning, TEXT("=== BONUS ROUND START ==="));

	CleanupBonusRound();

	BonusPhase = 1;  // Throwing
	BonusAnimTimer = 0.0f;
	BonusDiceModifier = 0;
	BonusLineupProgress = 0.0f;

	// Generate the enemy total (not 7)
	BonusEnemyTotal = GenerateBonusTotal();
	bBonusIsHigher = (BonusEnemyTotal > 7);

	UE_LOG(LogTemp, Warning, TEXT("Bonus Enemy Total: %d (%s than 7)"),
		BonusEnemyTotal, bBonusIsHigher ? TEXT("HIGHER") : TEXT("LOWER"));

	// Throw the masked dice
	ThrowBonusMaskedDice();
}

void ADiceGameManager::ThrowBonusMaskedDice()
{
	// Calculate dice values that sum to BonusEnemyTotal
	int32 Die1 = FMath::RandRange(1, FMath::Min(6, BonusEnemyTotal - 1));
	int32 Die2 = BonusEnemyTotal - Die1;
	Die1 = FMath::Clamp(Die1, 1, 6);
	Die2 = FMath::Clamp(Die2, 1, 6);

	// Use same spawn logic as normal enemy dice
	AMaskEnemy* Enemy = FindEnemy();
	FVector SpawnBase;
	if (Enemy)
	{
		SpawnBase = Enemy->GetActorLocation() + EnemyDiceSpawnOffset;
	}
	else
	{
		// Fallback if no enemy found
		SpawnBase = GetLineupWorldCenter() + FVector(0, 0, 100.0f);
	}

	FVector ThrowTarget = GetLineupWorldCenter();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	for (int32 i = 0; i < 2; i++)
	{
		int32 DieValue = (i == 0) ? Die1 : Die2;

		FVector SpawnOffset = FVector(
			FMath::RandRange(-15.0f, 15.0f),
			(i - 1.0f) * 20.0f,
			FMath::RandRange(0.0f, 10.0f)
		);
		FVector SpawnLocation = SpawnBase + SpawnOffset;
		FRotator SpawnRotation = FRotator(
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f),
			FMath::RandRange(0.0f, 360.0f)
		);

		ADice* Dice = GetWorld()->SpawnActor<ADice>(ADice::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
		if (Dice)
		{
			Dice->CurrentValue = DieValue;
			Dice->bShowDebugNumbers = bShowDebugGizmos;
			Dice->DiceSize = DiceScale;

			// Use masked mesh if set, otherwise use enemy mesh
			UStaticMesh* MeshToUse = MaskedDiceMesh ? MaskedDiceMesh : EnemyDiceMesh;
			if (MeshToUse)
			{
				Dice->SetCustomMesh(MeshToUse, CustomMeshScale);
			}
			UMaterialInterface* MatToUse = MaskedDiceMaterial ? MaskedDiceMaterial : EnemyDiceMaterial;
			if (MatToUse)
			{
				Dice->SetCustomMaterial(MatToUse);
			}
			Dice->SetFaceNumbersVisible(bShowDiceNumbers);
			Dice->SetTextColor(EnemyTextColor);

			// Throw towards table - same as normal enemy dice
			FVector ThrowDirection = (ThrowTarget - SpawnLocation).GetSafeNormal();
			ThrowDirection += FVector(
				FMath::RandRange(-0.15f, 0.15f),
				FMath::RandRange(-0.15f, 0.15f),
				FMath::RandRange(-0.1f, 0.0f)
			);
			Dice->Throw(ThrowDirection, DiceThrowForce);

			BonusMaskedDice.Add(Dice);

			UE_LOG(LogTemp, Warning, TEXT("Spawned masked dice %d at %s with value %d"),
				i, *SpawnLocation.ToString(), DieValue);
		}
	}

	if (SoundManager)
	{
		SoundManager->PlayDiceRoll();
	}
}

void ADiceGameManager::CheckBonusDiceSettled()
{
	bool bAllSettled = true;
	for (ADice* Dice : BonusMaskedDice)
	{
		if (Dice)
		{
			UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Dice->GetRootComponent());
			if (PrimComp)
			{
				FVector Vel = PrimComp->GetPhysicsLinearVelocity();
				FVector AngVel = PrimComp->GetPhysicsAngularVelocityInDegrees();
				if (Vel.Size() > 5.0f || AngVel.Size() > 10.0f)
				{
					bAllSettled = false;
					break;
				}
			}
		}
	}

	if (bAllSettled && BonusAnimTimer > 0.5f)
	{
		BonusPhase = 3;  // Lining up
		PrepareBonusDiceLineup();
	}
}

void ADiceGameManager::PrepareBonusDiceLineup()
{
	BonusLineupProgress = 0.0f;
	BonusDiceStartPositions.Empty();
	BonusDiceStartRotations.Empty();
	BonusDiceTargetPositions.Empty();
	BonusDiceTargetRotations.Empty();

	FVector WorldCenter = GetLineupWorldCenter();
	float LineupDir = FMath::DegreesToRadians(LineupYaw);
	FVector RightDir = FVector(FMath::Sin(LineupDir), FMath::Cos(LineupDir), 0.0f);
	FVector ForwardDir = FVector(FMath::Cos(LineupDir), -FMath::Sin(LineupDir), 0.0f);

	// Masked enemy dice go to ENEMY row (back, away from camera)
	FVector EnemyRowCenter = WorldCenter + ForwardDir * EnemyRowOffset;

	int32 NumDice = BonusMaskedDice.Num();
	float TotalWidth = (NumDice - 1) * DiceLineupSpacing;
	float StartOffset = -TotalWidth * 0.5f;

	for (int32 i = 0; i < NumDice; i++)
	{
		ADice* Dice = BonusMaskedDice[i];
		if (!Dice) continue;

		// Store current position
		BonusDiceStartPositions.Add(Dice->GetActorLocation());
		BonusDiceStartRotations.Add(Dice->GetActorRotation());

		// Calculate target
		float LateralOffset = StartOffset + i * DiceLineupSpacing;
		FVector TargetPos = EnemyRowCenter + RightDir * LateralOffset;
		TargetPos.Z = WorldCenter.Z + DiceLineupHeight;

		BonusDiceTargetPositions.Add(TargetPos);
		BonusDiceTargetRotations.Add(GetRotationForFaceUp(Dice->CurrentValue));

		// Disable physics
		UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Dice->GetRootComponent());
		if (PrimComp)
		{
			PrimComp->SetSimulatePhysics(false);
		}
	}
}

void ADiceGameManager::LineUpBonusDice(float DeltaTime)
{
	BonusLineupProgress += DeltaTime * DiceLineupSpeed;

	if (BonusLineupProgress >= 1.0f)
	{
		BonusLineupProgress = 1.0f;

		// Snap to final positions
		for (int32 i = 0; i < BonusMaskedDice.Num(); i++)
		{
			if (BonusMaskedDice[i] && i < BonusDiceTargetPositions.Num())
			{
				BonusMaskedDice[i]->SetActorLocation(BonusDiceTargetPositions[i]);
				BonusMaskedDice[i]->SetActorRotation(BonusDiceTargetRotations[i]);
			}
		}

		// Pan camera for player throw
		StartCameraPan();

		// Move to player throw phase
		BonusPhase = 4;
		BonusAnimTimer = 0.0f;

		// Throw player's YES dice
		ThrowBonusPlayerDice();
	}
	else
	{
		float T = EaseOutCubic(BonusLineupProgress);
		for (int32 i = 0; i < BonusMaskedDice.Num(); i++)
		{
			if (BonusMaskedDice[i] && i < BonusDiceStartPositions.Num())
			{
				FVector NewPos = FMath::Lerp(BonusDiceStartPositions[i], BonusDiceTargetPositions[i], T);
				FRotator NewRot = FMath::Lerp(BonusDiceStartRotations[i], BonusDiceTargetRotations[i], T);
				BonusMaskedDice[i]->SetActorLocation(NewPos);
				BonusMaskedDice[i]->SetActorRotation(NewRot);
			}
		}
	}
}

void ADiceGameManager::ThrowBonusPlayerDice()
{
	// Spawn from camera like normal player dice
	ADiceCamera* Cam = FindCamera();
	if (!Cam)
	{
		UE_LOG(LogTemp, Error, TEXT("No camera found for bonus player dice throw!"));
		return;
	}

	FVector CamLocation = Cam->Camera->GetComponentLocation();
	FVector CamForward = Cam->Camera->GetForwardVector();
	FVector SpawnBase = CamLocation + CamForward * 80.0f + FVector(0, 0, 30.0f);

	FVector SpawnLocation = SpawnBase + FVector(
		FMath::RandRange(-5.0f, 5.0f),
		FMath::RandRange(-5.0f, 5.0f),
		FMath::RandRange(-5.0f, 5.0f)
	);
	FRotator SpawnRotation = FRotator(
		FMath::RandRange(0.0f, 360.0f),
		FMath::RandRange(0.0f, 360.0f),
		FMath::RandRange(0.0f, 360.0f)
	);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	BonusPlayerDice = GetWorld()->SpawnActor<ADice>(ADice::StaticClass(), SpawnLocation, SpawnRotation, SpawnParams);
	if (BonusPlayerDice)
	{
		BonusPlayerDice->CurrentValue = 1;
		BonusPlayerDice->bShowDebugNumbers = bShowDebugGizmos;
		BonusPlayerDice->DiceSize = DiceScale;

		if (PlayerDiceMesh)
		{
			BonusPlayerDice->SetCustomMesh(PlayerDiceMesh, CustomMeshScale);
		}
		if (PlayerDiceMaterial)
		{
			BonusPlayerDice->SetCustomMaterial(PlayerDiceMaterial);
		}
		// Show "YES" text on all faces
		BonusPlayerDice->SetAllFacesText(TEXT("YES"));
		BonusPlayerDice->SetTextColor(PlayerTextColor);
		if (bPlayerDiceGlow)
		{
			BonusPlayerDice->SetGlowEnabled(true);
		}

		// Throw towards table like normal player dice
		FVector ThrowDirection = CamForward + FVector(0, 0, 0.1f);
		ThrowDirection += FVector(
			FMath::RandRange(-0.1f, 0.1f),
			FMath::RandRange(-0.1f, 0.1f),
			0
		);
		BonusPlayerDice->Throw(ThrowDirection, DiceThrowForce);

		UE_LOG(LogTemp, Warning, TEXT("Spawned bonus player YES dice at %s"), *SpawnLocation.ToString());
	}

	if (SoundManager)
	{
		SoundManager->PlayDiceRoll();
	}

	UE_LOG(LogTemp, Warning, TEXT("Player YES dice thrown!"));
}

void ADiceGameManager::CheckBonusPlayerDiceSettled()
{
	if (!BonusPlayerDice) return;

	UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(BonusPlayerDice->GetRootComponent());
	if (PrimComp)
	{
		FVector Vel = PrimComp->GetPhysicsLinearVelocity();
		FVector AngVel = PrimComp->GetPhysicsAngularVelocityInDegrees();
		if (Vel.Size() < 5.0f && AngVel.Size() < 10.0f && BonusAnimTimer > 0.5f)
		{
			BonusPhase = 6;  // Lineup player dice
			PrepareBonusPlayerLineup();
		}
	}
}

void ADiceGameManager::PrepareBonusPlayerLineup()
{
	BonusPlayerLineupProgress = 0.0f;

	if (!BonusPlayerDice) return;

	// Store current position
	BonusPlayerDiceStartPos = BonusPlayerDice->GetActorLocation();
	BonusPlayerDiceStartRot = BonusPlayerDice->GetActorRotation();

	// Target: centered in player row (front, close to camera)
	FVector WorldCenter = GetLineupWorldCenter();
	float LineupDir = FMath::DegreesToRadians(LineupYaw);
	FVector ForwardDir = FVector(FMath::Cos(LineupDir), -FMath::Sin(LineupDir), 0.0f);
	BonusPlayerDiceTargetPos = WorldCenter - ForwardDir * PlayerRowOffset;
	BonusPlayerDiceTargetPos.Z = WorldCenter.Z + DiceLineupHeight;

	// Face up with value 1
	BonusPlayerDiceTargetRot = GetRotationForFaceUp(1);

	// Disable physics
	UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(BonusPlayerDice->GetRootComponent());
	if (PrimComp)
	{
		PrimComp->SetSimulatePhysics(false);
	}
}

void ADiceGameManager::LineUpBonusPlayerDice(float DeltaTime)
{
	BonusPlayerLineupProgress += DeltaTime * DiceLineupSpeed;

	if (BonusPlayerLineupProgress >= 1.0f)
	{
		BonusPlayerLineupProgress = 1.0f;

		if (BonusPlayerDice)
		{
			BonusPlayerDice->SetActorLocation(BonusPlayerDiceTargetPos);
			BonusPlayerDice->SetActorRotation(BonusPlayerDiceTargetRot);
		}

		// Start player turn
		StartBonusPlayerTurn();
	}
	else
	{
		float T = EaseOutCubic(BonusPlayerLineupProgress);
		if (BonusPlayerDice)
		{
			FVector NewPos = FMath::Lerp(BonusPlayerDiceStartPos, BonusPlayerDiceTargetPos, T);
			FRotator NewRot = FMath::Lerp(BonusPlayerDiceStartRot, BonusPlayerDiceTargetRot, T);
			BonusPlayerDice->SetActorLocation(NewPos);
			BonusPlayerDice->SetActorRotation(NewRot);
		}
	}
}

void ADiceGameManager::StartBonusPlayerTurn()
{
	BonusPhase = 7;
	BonusAnimTimer = 0.0f;

	// Show ONLY bonus modifiers, hide all regular ones
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod)
		{
			if (Mod->ModifierType == EModifierType::BonusHigher ||
				Mod->ModifierType == EModifierType::BonusLower)
			{
				// Show and activate bonus modifiers (keep their placed position)
				Mod->SetHidden(false);
				Mod->SetActive(true);
				Mod->bIsUsed = false;
				// Reset text to original (in case it was changed to Lucky!/Unlucky)
				Mod->UpdateVisuals();
			}
			else
			{
				// Hide all regular modifiers during bonus round
				Mod->SetActive(false);
				Mod->SetHidden(true);
			}
		}
	}

	// Start the glitchy typewriter UI
	StartMasqueradeTypewriter();

	UE_LOG(LogTemp, Warning, TEXT("Drag YES dice to >7 or <7 modifier!"));
}

void ADiceGameManager::StartBonusDiceSnap(ADiceModifier* Modifier, bool bChoseHigher)
{
	if (!BonusPlayerDice || !Modifier) return;

	bBonusDiceSnapping = true;
	BonusSnapProgress = 0.0f;
	bBonusSnapChoseHigher = bChoseHigher;
	SelectedBonusModifier = Modifier;  // Store for later text update

	// Store start position
	BonusSnapStartPos = BonusPlayerDice->GetActorLocation();
	BonusSnapStartRot = BonusPlayerDice->GetActorRotation();

	// Target position is above the modifier
	BonusSnapTargetPos = Modifier->GetActorLocation() + FVector(0, 0, 20.0f);
	BonusSnapTargetRot = GetRotationForFaceUp(BonusPlayerDice->GetResult());
	BonusSnapTargetRot.Yaw += LineupYaw;

	// Disable physics during snap
	if (BonusPlayerDice->Mesh)
	{
		BonusPlayerDice->Mesh->SetSimulatePhysics(false);
	}

	UE_LOG(LogTemp, Warning, TEXT("Starting bonus dice snap to modifier"));
}

void ADiceGameManager::UpdateBonusDiceSnap(float DeltaTime)
{
	if (!bBonusDiceSnapping) return;
	if (!BonusPlayerDice)
	{
		bBonusDiceSnapping = false;
		return;
	}

	BonusSnapProgress += DeltaTime * 5.0f;  // Same speed as normal snap
	float Alpha = FMath::Clamp(BonusSnapProgress, 0.0f, 1.0f);

	// Smooth easing
	float SmoothAlpha = EaseOutCubic(Alpha);

	// Lerp position and rotation
	FVector NewPos = FMath::Lerp(BonusSnapStartPos, BonusSnapTargetPos, SmoothAlpha);
	FRotator NewRot = FMath::Lerp(BonusSnapStartRot, BonusSnapTargetRot, SmoothAlpha);
	BonusPlayerDice->SetActorLocation(NewPos);
	BonusPlayerDice->SetActorRotation(NewRot);

	if (BonusSnapProgress >= 1.0f)
	{
		// Snap complete - set final position
		BonusPlayerDice->SetActorLocation(BonusSnapTargetPos);
		BonusPlayerDice->SetActorRotation(BonusSnapTargetRot);

		bBonusDiceSnapping = false;

		// Now process the choice
		OnBonusModifierSelected(bBonusSnapChoseHigher);
	}
}

void ADiceGameManager::StartBonusCameraShake(float Duration, float Intensity)
{
	bBonusCameraShaking = true;
	BonusCameraShakeTimer = 0.0f;
	BonusCameraShakeDuration = Duration;
	BonusCameraShakeIntensity = Intensity;
	BonusCameraShakeOffset = FVector::ZeroVector;
}

void ADiceGameManager::UpdateBonusCameraShake(float DeltaTime)
{
	if (!bBonusCameraShaking) return;

	ADiceCamera* Cam = FindCamera();
	if (!Cam) return;

	BonusCameraShakeTimer += DeltaTime;

	if (BonusCameraShakeTimer >= BonusCameraShakeDuration)
	{
		// Shake complete - remove offset
		bBonusCameraShaking = false;
		Cam->SetActorLocation(Cam->GetActorLocation() - BonusCameraShakeOffset);
		BonusCameraShakeOffset = FVector::ZeroVector;
		return;
	}

	// Remove old offset
	FVector CurrentPos = Cam->GetActorLocation() - BonusCameraShakeOffset;

	// Calculate new shake offset (decay over time)
	float Progress = BonusCameraShakeTimer / BonusCameraShakeDuration;
	float DecayedIntensity = BonusCameraShakeIntensity * (1.0f - Progress);

	BonusCameraShakeOffset = FVector(
		FMath::RandRange(-DecayedIntensity, DecayedIntensity),
		FMath::RandRange(-DecayedIntensity, DecayedIntensity),
		FMath::RandRange(-DecayedIntensity * 0.5f, DecayedIntensity * 0.5f)
	);

	// Apply new offset
	Cam->SetActorLocation(CurrentPos + BonusCameraShakeOffset);
}

void ADiceGameManager::OnBonusModifierSelected(bool bHigher)
{
	bPlayerGuessedHigher = bHigher;
	bBonusWon = (bPlayerGuessedHigher == bBonusIsHigher);

	UE_LOG(LogTemp, Warning, TEXT("Player chose: %s"), bHigher ? TEXT(">7") : TEXT("<7"));

	// Type out the masquerade UI (glitchy reverse effect)
	StartMasqueradeTypewriterOut();

	// Deactivate modifiers
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && (Mod->ModifierType == EModifierType::BonusHigher ||
					Mod->ModifierType == EModifierType::BonusLower))
		{
			Mod->SetActive(false);
		}
	}

	// Move to reveal phase - start the juicy reveal sequence
	BonusPhase = 8;
	BonusAnimTimer = 0.0f;
	StartBonusRevealSequence();
}

void ADiceGameManager::StartBonusRevealSequence()
{
	// Phase 0: Start shaking the masked dice
	BonusRevealPhase = 0;
	RevealShakeTimer = 0.0f;
	BonusRevealProgress = 0.0f;
	RevealStrikeProgress = 0.0f;

	// Store original positions for shake
	MaskedDicePreShakePos.Empty();
	MaskedDiceFlyVelocity.Empty();
	for (ADice* Dice : BonusMaskedDice)
	{
		if (Dice)
		{
			MaskedDicePreShakePos.Add(Dice->GetActorLocation());
			MaskedDiceFlyVelocity.Add(FVector::ZeroVector);
		}
	}

	// Spawn the reveal dice (with actual number) off-screen, ready to strike
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector WorldCenter = GetLineupWorldCenter();
	float LineupDir = FMath::DegreesToRadians(LineupYaw);
	FVector ForwardDir = FVector(FMath::Cos(LineupDir), -FMath::Sin(LineupDir), 0.0f);

	// Start position: far behind, will strike toward masked dice
	RevealDiceStartPos = WorldCenter + ForwardDir * 200.0f;
	RevealDiceStartPos.Z = WorldCenter.Z + 50.0f;

	// Target: center of masked dice
	RevealDiceTargetPos = WorldCenter + ForwardDir * EnemyRowOffset;
	RevealDiceTargetPos.Z = WorldCenter.Z + DiceLineupHeight;

	BonusRevealDice = GetWorld()->SpawnActor<ADice>(ADice::StaticClass(), RevealDiceStartPos, FRotator::ZeroRotator, SpawnParams);
	if (BonusRevealDice)
	{
		// Use ENEMY mesh (with visible numbers), not masked
		if (EnemyDiceMesh)
		{
			BonusRevealDice->SetCustomMesh(EnemyDiceMesh, CustomMeshScale);
		}
		if (EnemyDiceMaterial)
		{
			BonusRevealDice->SetCustomMaterial(EnemyDiceMaterial);
		}
		// Setup like normal enemy dice - show the total sum on ALL faces
		BonusRevealDice->bShowDebugNumbers = false;  // No debug overlay
		BonusRevealDice->DiceSize = DiceScale;
		BonusRevealDice->SetTextSettings(DiceTextSize, DiceTextOffset);  // Must be before SetAllFacesText
		BonusRevealDice->SetTextColor(EnemyTextColor);
		BonusRevealDice->SetAllFacesText(FString::FromInt(BonusEnemyTotal));
		BonusRevealDice->SetFaceNumbersVisible(true);  // Always show the number
		BonusRevealDice->SetActorHiddenInGame(true);  // Hidden until strike
	}

	UE_LOG(LogTemp, Warning, TEXT("Starting bonus reveal sequence. Total: %d"), BonusEnemyTotal);
}

void ADiceGameManager::UpdateBonusReveal(float DeltaTime)
{
	BonusRevealProgress += DeltaTime;

	FVector WorldCenter = GetLineupWorldCenter();
	float LineupDir = FMath::DegreesToRadians(LineupYaw);
	FVector ForwardDir = FVector(FMath::Cos(LineupDir), -FMath::Sin(LineupDir), 0.0f);
	FVector RightDir = FVector(FMath::Sin(LineupDir), FMath::Cos(LineupDir), 0.0f);

	switch (BonusRevealPhase)
	{
		case 0:  // Shake masked dice
		{
			RevealShakeTimer += DeltaTime;
			float ShakeDuration = 0.8f;

			// Shake intensity increases
			float ShakeIntensity = FMath::Min(RevealShakeTimer / ShakeDuration, 1.0f) * 8.0f;

			for (int32 i = 0; i < BonusMaskedDice.Num(); i++)
			{
				if (BonusMaskedDice[i] && MaskedDicePreShakePos.IsValidIndex(i))
				{
					FVector ShakeOffset = FVector(
						FMath::RandRange(-ShakeIntensity, ShakeIntensity),
						FMath::RandRange(-ShakeIntensity, ShakeIntensity),
						FMath::RandRange(0.0f, ShakeIntensity * 0.5f)
					);
					BonusMaskedDice[i]->SetActorLocation(MaskedDicePreShakePos[i] + ShakeOffset);
				}
			}

			if (RevealShakeTimer >= ShakeDuration)
			{
				// Transition to strike phase
				BonusRevealPhase = 1;
				RevealStrikeProgress = 0.0f;
				if (BonusRevealDice)
				{
					BonusRevealDice->SetActorHiddenInGame(false);
				}
			}
			break;
		}

		case 1:  // Strike incoming - reveal dice flies in
		{
			RevealStrikeProgress += DeltaTime * 4.0f;  // Fast!

			if (BonusRevealDice)
			{
				float T = FMath::Clamp(RevealStrikeProgress, 0.0f, 1.0f);
				FVector NewPos = FMath::Lerp(RevealDiceStartPos, RevealDiceTargetPos, T);

				// Add rotation during flight
				FRotator NewRot = BonusRevealDice->GetActorRotation();
				NewRot.Pitch += DeltaTime * 1500.0f;
				NewRot.Yaw += DeltaTime * 800.0f;

				BonusRevealDice->SetActorLocation(NewPos);
				BonusRevealDice->SetActorRotation(NewRot);
			}

			if (RevealStrikeProgress >= 1.0f)
			{
				// Impact! Camera shake on hit
				StartBonusCameraShake(0.4f, 8.0f);

				// Calculate fly velocities for masked dice
				for (int32 i = 0; i < BonusMaskedDice.Num(); i++)
				{
					if (BonusMaskedDice[i])
					{
						FVector DicePos = BonusMaskedDice[i]->GetActorLocation();
						FVector ImpactDir = (DicePos - RevealDiceTargetPos).GetSafeNormal();
						ImpactDir.Z = FMath::RandRange(0.3f, 0.6f);
						ImpactDir += RightDir * FMath::RandRange(-0.5f, 0.5f);
						ImpactDir.Normalize();

						float FlySpeed = FMath::RandRange(400.0f, 600.0f);
						if (MaskedDiceFlyVelocity.IsValidIndex(i))
						{
							MaskedDiceFlyVelocity[i] = ImpactDir * FlySpeed;
						}
					}
				}

				BonusRevealPhase = 2;
				BonusAnimTimer = 0.0f;

				if (SoundManager) SoundManager->PlayDiceRoll();
			}
			break;
		}

		case 2:  // Impact - masked dice fly away, reveal dice settles
		{
			BonusAnimTimer += DeltaTime;

			// Fly masked dice away
			for (int32 i = 0; i < BonusMaskedDice.Num(); i++)
			{
				if (BonusMaskedDice[i] && MaskedDiceFlyVelocity.IsValidIndex(i))
				{
					FVector Pos = BonusMaskedDice[i]->GetActorLocation();
					FVector Vel = MaskedDiceFlyVelocity[i];

					// Apply gravity
					Vel.Z -= 800.0f * DeltaTime;
					MaskedDiceFlyVelocity[i] = Vel;

					Pos += Vel * DeltaTime;
					BonusMaskedDice[i]->SetActorLocation(Pos);

					// Spin
					FRotator Rot = BonusMaskedDice[i]->GetActorRotation();
					Rot.Pitch += DeltaTime * 600.0f;
					Rot.Roll += DeltaTime * 400.0f;
					BonusMaskedDice[i]->SetActorRotation(Rot);
				}
			}

			// Reveal dice settles with bounce
			if (BonusRevealDice)
			{
				float SettleT = FMath::Clamp(BonusAnimTimer * 2.0f, 0.0f, 1.0f);
				float Bounce = FMath::Abs(FMath::Sin(SettleT * PI * 3.0f)) * (1.0f - SettleT) * 15.0f;

				FVector SettlePos = RevealDiceTargetPos;
				SettlePos.Z += Bounce;
				BonusRevealDice->SetActorLocation(SettlePos);

				// Settle rotation - all faces show the same number, so just use face 1
				FRotator TargetRot = GetRotationForFaceUp(1);
				TargetRot.Yaw += LineupYaw;
				FRotator CurrentRot = BonusRevealDice->GetActorRotation();
				BonusRevealDice->SetActorRotation(FMath::Lerp(CurrentRot, TargetRot, DeltaTime * 5.0f));
			}

			if (BonusAnimTimer >= 1.0f)
			{
				BonusRevealPhase = 3;
				BonusAnimTimer = 0.0f;
				BonusBounceCount = 0;

				// Store start positions for both dice
				if (BonusPlayerDice)
				{
					BonusPlayerDiceStartPos = BonusPlayerDice->GetActorLocation();
				}
				if (BonusRevealDice)
				{
					BonusRevealDiceStartPos = BonusRevealDice->GetActorLocation();
				}

				// Floor level for bouncing
				BonusResultFloorZ = WorldCenter.Z + DiceLineupHeight;

				// Calculate launch direction and velocity - arc trajectory
				FVector LaunchDir;
				if (bBonusWon)
				{
					// WON: Launch toward player (back toward camera)
					LaunchDir = -ForwardDir;
				}
				else
				{
					// LOST: Launch toward enemy (away from camera)
					LaunchDir = ForwardDir;
				}

				// Give both dice initial velocity - arc upward then forward
				float LaunchSpeed = 250.0f;
				float LaunchUpward = 350.0f;

				BonusPlayerVelocity = LaunchDir * LaunchSpeed + FVector(0, 0, LaunchUpward);
				BonusRevealVelocity = LaunchDir * (LaunchSpeed * 0.9f) + FVector(0, 0, LaunchUpward * 1.1f);

				// Add some sideways spread
				BonusPlayerVelocity += RightDir * FMath::RandRange(-40.0f, 40.0f);
				BonusRevealVelocity += RightDir * FMath::RandRange(-40.0f, 40.0f);

				// Rotation speeds
				BonusPlayerRotSpeed = bBonusWon ? 400.0f : 600.0f;
				BonusRevealRotSpeed = bBonusWon ? 350.0f : 500.0f;

				// Update modifier text to Lucky!/Unlucky :(
				if (SelectedBonusModifier && SelectedBonusModifier->ModifierText)
				{
					if (bBonusWon)
					{
						SelectedBonusModifier->ModifierText->SetText(FText::FromString(TEXT("Lucky!")));
						SelectedBonusModifier->ModifierText->SetTextRenderColor(FColor::Green);
					}
					else
					{
						SelectedBonusModifier->ModifierText->SetText(FText::FromString(TEXT("Unlucky :(")));
						SelectedBonusModifier->ModifierText->SetTextRenderColor(FColor::Red);
					}
				}

				// Play appropriate sound and camera shake
				if (bBonusWon)
				{
					StartBonusCameraShake(0.3f, 6.0f);
					if (SoundManager) SoundManager->PlayDiceMatch();
				}
				else
				{
					StartBonusCameraShake(0.5f, 12.0f);
					if (SoundManager) SoundManager->PlayError();
				}
			}
			break;
		}

		case 3:  // Result animation - physics-based arc with bouncing
		{
			BonusAnimTimer += DeltaTime;

			float Gravity = 800.0f;
			float BounceDamping = 0.6f;
			float FrictionDamping = 0.98f;

			// Update player dice with physics
			if (BonusPlayerDice)
			{
				FVector Pos = BonusPlayerDice->GetActorLocation();

				// Apply gravity
				BonusPlayerVelocity.Z -= Gravity * DeltaTime;

				// Apply velocity
				Pos += BonusPlayerVelocity * DeltaTime;

				// Bounce off floor
				if (Pos.Z < BonusResultFloorZ)
				{
					Pos.Z = BonusResultFloorZ;
					BonusPlayerVelocity.Z = -BonusPlayerVelocity.Z * BounceDamping;
					BonusPlayerVelocity.X *= FrictionDamping;
					BonusPlayerVelocity.Y *= FrictionDamping;
					BonusPlayerRotSpeed *= 0.7f;

					// Camera shake on bounce
					if (FMath::Abs(BonusPlayerVelocity.Z) > 50.0f)
					{
						StartBonusCameraShake(0.15f, 3.0f);
						if (SoundManager) SoundManager->PlayDiceRoll();
					}
				}

				BonusPlayerDice->SetActorLocation(Pos);

				// Rotation - tumble based on velocity
				FRotator Rot = BonusPlayerDice->GetActorRotation();
				float RotAmount = BonusPlayerRotSpeed * DeltaTime;
				if (bBonusWon)
				{
					Rot.Yaw += RotAmount;
					Rot.Pitch += RotAmount * 0.3f;
				}
				else
				{
					Rot.Roll += RotAmount;
					Rot.Pitch += RotAmount * 0.5f;
				}
				BonusPlayerDice->SetActorRotation(Rot);
			}

			// Update reveal dice with physics
			if (BonusRevealDice)
			{
				FVector Pos = BonusRevealDice->GetActorLocation();

				// Apply gravity
				BonusRevealVelocity.Z -= Gravity * DeltaTime;

				// Apply velocity
				Pos += BonusRevealVelocity * DeltaTime;

				// Bounce off floor
				if (Pos.Z < BonusResultFloorZ)
				{
					Pos.Z = BonusResultFloorZ;
					BonusRevealVelocity.Z = -BonusRevealVelocity.Z * BounceDamping;
					BonusRevealVelocity.X *= FrictionDamping;
					BonusRevealVelocity.Y *= FrictionDamping;
					BonusRevealRotSpeed *= 0.7f;
				}

				BonusRevealDice->SetActorLocation(Pos);

				// Rotation
				FRotator Rot = BonusRevealDice->GetActorRotation();
				float RotAmount = BonusRevealRotSpeed * DeltaTime;
				Rot.Yaw += RotAmount * 0.8f;
				Rot.Roll += RotAmount * 0.4f;
				BonusRevealDice->SetActorRotation(Rot);
			}

			// Transition when dice have mostly settled (low velocity)
			bool bSettled = (BonusPlayerVelocity.Size() < 30.0f && BonusRevealVelocity.Size() < 30.0f)
						 || BonusAnimTimer >= 2.0f;

			if (bSettled)
			{
				// Move to camera pan out phase
				BonusRevealPhase = 4;
				BonusAnimTimer = 0.0f;

				// Start camera return
				StartBonusButtonCameraReturn();
			}
			break;
		}

		case 4:  // Camera pan out, then finish
		{
			BonusAnimTimer += DeltaTime;

			// Wait for camera to mostly finish returning
			if (BonusAnimTimer >= 1.0f)
			{
				ShowBonusResult();
			}
			break;
		}
	}
}

void ADiceGameManager::ShowBonusResult()
{
	BonusPhase = 9;
	BonusAnimTimer = 0.0f;

	if (bBonusWon)
	{
		BonusDiceModifier = 1;
		UE_LOG(LogTemp, Warning, TEXT("BONUS WIN! +1 dice for next round. Total was %d (%s than 7)"),
			BonusEnemyTotal, bBonusIsHigher ? TEXT("HIGHER") : TEXT("LOWER"));
		if (SoundManager) SoundManager->PlayDiceMatch();

		// Pop effect on all dice
		PlayMatchEffect(BonusRevealDice ? BonusRevealDice->GetActorLocation() : GetLineupWorldCenter());
	}
	else
	{
		BonusDiceModifier = -1;
		UE_LOG(LogTemp, Warning, TEXT("BONUS LOSE! -1 dice for next round. Total was %d (%s than 7)"),
			BonusEnemyTotal, bBonusIsHigher ? TEXT("HIGHER") : TEXT("LOWER"));
		if (SoundManager) SoundManager->PlayError();
	}

	// Apply modifier for NEXT ROUND ONLY (temporary bonus)
	// Base is always 5, bonus gives 6 (win) or 4 (lose)
	PlayerNumDice = FMath::Clamp(BaseDiceCount + BonusDiceModifier, 4, 6);
	bBonusActiveThisRound = true;  // Flag that next round uses bonus, will reset after
}

void ADiceGameManager::EndBonusRound(bool bWon)
{
	// Legacy function - now handled by ShowBonusResult
	bBonusWon = bWon;
	ShowBonusResult();
}

void ADiceGameManager::UpdateBonusRound(float DeltaTime)
{
	if (BonusPhase == 0) return;

	BonusAnimTimer += DeltaTime;

	switch (BonusPhase)
	{
		case 1:  // Enemy throwing - wait a moment
			if (BonusAnimTimer > 0.2f)
			{
				BonusPhase = 2;  // Enemy settling
				BonusAnimTimer = 0.0f;
			}
			break;

		case 2:  // Enemy dice settling
			CheckBonusDiceSettled();
			break;

		case 3:  // Enemy dice lining up
			LineUpBonusDice(DeltaTime);
			break;

		case 4:  // Player dice throwing
			if (BonusAnimTimer > 0.2f)
			{
				BonusPhase = 5;  // Player settling
				BonusAnimTimer = 0.0f;
			}
			break;

		case 5:  // Player dice settling
			CheckBonusPlayerDiceSettled();
			break;

		case 6:  // Player dice lining up (centering)
			LineUpBonusPlayerDice(DeltaTime);
			break;

		case 7:  // Player turn - waiting for drag to modifier
			// Handled by drag system in OnMouseReleased
			break;

		case 8:  // Reveal dice animation
			UpdateBonusReveal(DeltaTime);
			break;

		case 9:  // Result pop and finish
			if (BonusAnimTimer > 1.5f)
			{
				CleanupBonusRound();
				BonusPhase = 0;
				ContinueToNextRound();
			}
			break;
	}
}

void ADiceGameManager::CleanupBonusRound()
{
	// Mark that bonus round just ended - ContinueToNextRound will handle dice reset logic
	bBonusRoundJustEnded = true;

	// Type out the masquerade UI if still visible
	StartMasqueradeTypewriterOut();

	// Destroy masked dice
	for (ADice* Dice : BonusMaskedDice)
	{
		if (Dice)
		{
			Dice->Destroy();
		}
	}
	BonusMaskedDice.Empty();

	// Destroy player dice
	if (BonusPlayerDice)
	{
		BonusPlayerDice->Destroy();
		BonusPlayerDice = nullptr;
	}

	// Destroy reveal dice
	if (BonusRevealDice)
	{
		BonusRevealDice->Destroy();
		BonusRevealDice = nullptr;
	}

	// Clear reveal animation state
	MaskedDicePreShakePos.Empty();
	MaskedDiceFlyVelocity.Empty();
	BonusRevealPhase = 0;
	bBonusDiceSnapping = false;
	SelectedBonusModifier = nullptr;
	BonusPlayerVelocity = FVector::ZeroVector;
	BonusRevealVelocity = FVector::ZeroVector;

	// Stop camera shake if active
	if (bBonusCameraShaking)
	{
		ADiceCamera* Cam = FindCamera();
		if (Cam)
		{
			Cam->SetActorLocation(Cam->GetActorLocation() - BonusCameraShakeOffset);
		}
		bBonusCameraShaking = false;
		BonusCameraShakeOffset = FVector::ZeroVector;
	}

	// Hide bonus modifiers, restore regular modifiers
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod)
		{
			if (Mod->ModifierType == EModifierType::BonusHigher ||
				Mod->ModifierType == EModifierType::BonusLower)
			{
				// Hide bonus modifiers
				Mod->SetActive(false);
				Mod->SetHidden(true);
			}
			else
			{
				// Show regular modifiers again
				Mod->SetHidden(false);
			}
		}
	}

	BonusDiceStartPositions.Empty();
	BonusDiceStartRotations.Empty();
	BonusDiceTargetPositions.Empty();
	BonusDiceTargetRotations.Empty();
}

// ==================== MASQUERADE UI TYPEWRITER ====================

void ADiceGameManager::StartMasqueradeTypewriter()
{
	if (!MasqueradeUIActor)
	{
		return;
	}

	// Find the text render component if not cached
	if (!MasqueradeUIText)
	{
		MasqueradeUIText = MasqueradeUIActor->FindComponentByClass<UTextRenderComponent>();
	}

	if (!MasqueradeUIText)
	{
		return;
	}

	// Store the full text from the component's current text
	MasqueradeFullText = MasqueradeUIText->Text.ToString();
	MasqueradeCurrentText = TEXT("");
	TypewriterTimer = 0.0f;
	TypewriterIndex = 0;
	GlitchTimer = 0.0f;
	bShowingGlitch = false;
	bTypewriterActive = true;
	bTypewriterOut = false;  // Typing IN

	// Show the actor and clear the text
	MasqueradeUIActor->SetActorHiddenInGame(false);
	MasqueradeUIText->SetText(FText::FromString(TEXT("")));

	UE_LOG(LogTemp, Warning, TEXT("Starting Masquerade typewriter IN: %s"), *MasqueradeFullText);
}

void ADiceGameManager::StartMasqueradeTypewriterOut()
{
	if (!MasqueradeUIActor || !MasqueradeUIText)
	{
		return;
	}

	// If already typing out or not active, just hide
	if (bTypewriterOut || MasqueradeCurrentText.Len() == 0)
	{
		HideMasqueradeUI();
		return;
	}

	// Start typing out from current text
	TypewriterTimer = 0.0f;
	TypewriterIndex = MasqueradeCurrentText.Len();
	GlitchTimer = 0.0f;
	bShowingGlitch = false;
	bTypewriterActive = true;
	bTypewriterOut = true;  // Typing OUT

	UE_LOG(LogTemp, Warning, TEXT("Starting Masquerade typewriter OUT"));
}

void ADiceGameManager::UpdateMasqueradeTypewriter(float DeltaTime)
{
	if (!bTypewriterActive || !MasqueradeUIText)
	{
		return;
	}

	TypewriterTimer += DeltaTime;
	float CharInterval = 1.0f / (TypewriterSpeed * 1.5f);  // Faster for out effect

	// Handle glitch effect
	if (bShowingGlitch)
	{
		GlitchTimer += DeltaTime;
		if (GlitchTimer > 0.04f)  // Glitch duration
		{
			bShowingGlitch = false;
			GlitchTimer = 0.0f;
			MasqueradeUIText->SetText(FText::FromString(MasqueradeCurrentText));
		}
		else
		{
			// Show glitchy text
			if (MasqueradeCurrentText.Len() > 0)
			{
				FString GlitchText = MasqueradeCurrentText;
				int32 GlitchPos = FMath::RandRange(0, FMath::Max(0, GlitchText.Len() - 1));
				int32 GlitchCharIdx = FMath::RandRange(0, GlitchChars.Len() - 1);
				GlitchText[GlitchPos] = GlitchChars[GlitchCharIdx];
				// Randomly add extra glitch chars
				if (FMath::FRand() < 0.3f)
				{
					GlitchText.AppendChar(GlitchChars[FMath::RandRange(0, GlitchChars.Len() - 1)]);
				}
				MasqueradeUIText->SetText(FText::FromString(GlitchText));
			}
		}
		return;
	}

	if (TypewriterTimer >= CharInterval)
	{
		TypewriterTimer = 0.0f;

		if (bTypewriterOut)
		{
			// TYPING OUT - remove characters
			if (MasqueradeCurrentText.Len() > 0)
			{
				MasqueradeCurrentText = MasqueradeCurrentText.Left(MasqueradeCurrentText.Len() - 1);
				TypewriterIndex = MasqueradeCurrentText.Len();

				// Chance to trigger glitch effect
				if (FMath::FRand() < GlitchChance * 1.5f)  // More glitchy when typing out
				{
					bShowingGlitch = true;
					GlitchTimer = 0.0f;
				}
				else
				{
					MasqueradeUIText->SetText(FText::FromString(MasqueradeCurrentText));
				}

				// Typewriter out complete
				if (MasqueradeCurrentText.Len() == 0)
				{
					bTypewriterActive = false;
					HideMasqueradeUI();
				}
			}
		}
		else
		{
			// TYPING IN - add characters
			if (TypewriterIndex < MasqueradeFullText.Len())
			{
				MasqueradeCurrentText.AppendChar(MasqueradeFullText[TypewriterIndex]);
				TypewriterIndex++;

				// Chance to trigger glitch effect
				if (FMath::FRand() < GlitchChance)
				{
					bShowingGlitch = true;
					GlitchTimer = 0.0f;
				}
				else
				{
					MasqueradeUIText->SetText(FText::FromString(MasqueradeCurrentText));
				}

				// Typewriter in complete
				if (TypewriterIndex >= MasqueradeFullText.Len())
				{
					bTypewriterActive = false;
					MasqueradeUIText->SetText(FText::FromString(MasqueradeFullText));
				}
			}
		}
	}
}

// ==================== WIN SEQUENCE ====================

void ADiceGameManager::StartWinSequence()
{
	WinSequencePhase = 1;  // Start with modifier fade
	WinSequenceTimer = 0.0f;
	WinSequenceProgress = 0.0f;
	WinMaskRotationProgress = 0.0f;
	WinFadeAlpha = 0.0f;

	// Store mask MESH starting position (we move the mesh, not the actor)
	AMaskEnemy* Enemy = FindEnemy();
	if (Enemy && Enemy->Mesh)
	{
		// Get current relative position as start
		WinMaskMeshStartPos = Enemy->Mesh->GetRelativeLocation();
		WinMaskMeshStartRot = Enemy->Mesh->GetRelativeRotation();

		// Target position - close to player (hardcoded relative position)
		// Based on: Start X=1340 -> Target X=652 (moves ~688 units toward player)
		WinMaskMeshTargetPos = FVector(652.0f, WinMaskMeshStartPos.Y, 215.0f);
		WinMaskMeshTargetRot = FRotator(0.0f, 270.0f, 0.0f);  // Rotates from ~90 to ~270 (180 degree turn)

		UE_LOG(LogTemp, Warning, TEXT("WIN: Mask start pos: %s, target pos: %s"),
			*WinMaskMeshStartPos.ToString(), *WinMaskMeshTargetPos.ToString());
	}

	// Setup win camera breathing
	ADiceCamera* Cam = FindCamera();
	if (Cam)
	{
		bWinCameraBreathing = true;
		WinCameraBreathTimer = 0.0f;
		WinCameraBasePos = Cam->GetActorLocation();
		WinCameraBaseRot = Cam->GetActorRotation();
	}

	// Initialize modifier fade alphas
	ModifierFadeAlpha.Empty();
	for (int32 i = 0; i < AllModifiers.Num(); i++)
	{
		ModifierFadeAlpha.Add(1.0f);  // Start fully visible
	}

	// Update timer text to show victory
	URoundTimerComponent* Timer = GetRoundTimer();
	if (Timer)
	{
		Timer->SetState(ETimerState::TimeUp);  // Use TimeUp state
	}

	// Clear all dice immediately
	for (ADice* Dice : EnemyDice)
	{
		if (Dice) Dice->Destroy();
	}
	EnemyDice.Empty();
	for (ADice* Dice : PlayerDice)
	{
		if (Dice) Dice->Destroy();
	}
	PlayerDice.Empty();

	// Clear results arrays to prevent crashes
	EnemyResults.Empty();
	PlayerResults.Empty();
	PlayerDiceMatched.Empty();
	EnemyDiceMatched.Empty();

	// Stop any dragging
	bIsDragging = false;
	DraggedDice = nullptr;
	DraggedDiceIndex = -1;

	// Create the fade widget (starts invisible)
	CreateFadeWidget();

	// Victory camera shake!
	StartBonusCameraShake(0.6f, 15.0f);

	// Play victory sound
	if (SoundManager) SoundManager->PlayDiceMatch();

	UE_LOG(LogTemp, Warning, TEXT("WIN SEQUENCE STARTED!"));
}

void ADiceGameManager::UpdateWinSequence(float DeltaTime)
{
	if (WinSequencePhase == 0) return;

	WinSequenceTimer += DeltaTime;

	// Update win camera breathing while sequence is active
	if (bWinCameraBreathing)
	{
		ADiceCamera* Cam = FindCamera();
		if (Cam)
		{
			WinCameraBreathTimer += DeltaTime;

			// Fast, intense breathing - player is excited/nervous
			float BreathSpeed = 4.0f;  // Much faster than normal
			float BreathIntensity = 8.0f;  // Stronger movement

			// Multiple sine waves for organic breathing
			float Breath1 = FMath::Sin(WinCameraBreathTimer * BreathSpeed) * BreathIntensity;
			float Breath2 = FMath::Sin(WinCameraBreathTimer * BreathSpeed * 1.3f) * (BreathIntensity * 0.4f);

			FVector BreathOffset = FVector(0, 0, Breath1 + Breath2);

			// Add slight forward/back motion
			float ForwardBreath = FMath::Sin(WinCameraBreathTimer * BreathSpeed * 0.7f) * 3.0f;
			BreathOffset.X = ForwardBreath;

			Cam->SetActorLocation(WinCameraBasePos + BreathOffset);

			// Subtle rotation shake
			FRotator BreathRot = WinCameraBaseRot;
			BreathRot.Pitch += FMath::Sin(WinCameraBreathTimer * BreathSpeed * 0.8f) * 1.5f;
			BreathRot.Roll += FMath::Sin(WinCameraBreathTimer * BreathSpeed * 1.1f) * 0.8f;
			Cam->SetActorRotation(BreathRot);
		}
	}

	switch (WinSequencePhase)
	{
		case 1:  // Modifiers fade out (or wait if no modifiers)
		{
			UpdateModifierFade(DeltaTime);

			// Check if all modifiers faded - require at least 1 second even if no modifiers
			bool bAllFaded = true;
			for (float Alpha : ModifierFadeAlpha)
			{
				if (Alpha > 0.01f)
				{
					bAllFaded = false;
					break;
				}
			}

			// Wait minimum 1.5 seconds before moving to mask float phase
			if ((bAllFaded && WinSequenceTimer >= 1.0f) || WinSequenceTimer >= 2.0f)
			{
				// Hide all modifiers
				for (ADiceModifier* Mod : AllModifiers)
				{
					if (Mod) Mod->SetHidden(true);
				}

				WinSequencePhase = 2;
				WinSequenceTimer = 0.0f;
				WinSequenceProgress = 0.0f;
				WinFadeAlpha = 0.0f;  // Ensure fade starts at 0

				// Play victory sound
				if (SoundManager) SoundManager->PlayDiceMatch();

				UE_LOG(LogTemp, Warning, TEXT("WIN: Moving to mask float phase"));
			}
			break;
		}

		case 2:  // Mask floats toward camera + rotates + screen fades (all simultaneous)
		{
			UpdateMaskFloat(DeltaTime);

			// When mask is close enough and faded, finish
			if (WinSequenceProgress >= 1.0f && WinFadeAlpha >= 0.95f)
			{
				WinSequencePhase = 3;
				WinSequenceTimer = 0.0f;
				bWinCameraBreathing = false;  // Stop breathing
				OnWinSequenceComplete();
			}
			break;
		}

		case 3:  // Done - wait for player input
		{
			// Could add "Press any key" logic here
			break;
		}
	}
}

void ADiceGameManager::UpdateModifierFade(float DeltaTime)
{
	float FadeSpeed = 1.5f;

	for (int32 i = 0; i < AllModifiers.Num(); i++)
	{
		if (AllModifiers[i] && ModifierFadeAlpha.IsValidIndex(i))
		{
			// Stagger the fade based on index
			float Delay = i * 0.1f;
			if (WinSequenceTimer > Delay)
			{
				ModifierFadeAlpha[i] = FMath::Max(0.0f, ModifierFadeAlpha[i] - DeltaTime * FadeSpeed);

				// Apply fade to modifier text
				if (AllModifiers[i]->ModifierText)
				{
					uint8 Alpha = static_cast<uint8>(ModifierFadeAlpha[i] * 255.0f);
					FColor CurrentColor = AllModifiers[i]->ModifierText->TextRenderColor;
					CurrentColor.A = Alpha;
					AllModifiers[i]->ModifierText->SetTextRenderColor(CurrentColor);
				}
			}
		}
	}
}

void ADiceGameManager::UpdateMaskFloat(float DeltaTime)
{
	AMaskEnemy* Enemy = FindEnemy();
	if (!Enemy || !Enemy->Mesh) return;

	float FloatDuration = 6.0f;  // Total time for mask to reach player (slow and dramatic)
	WinSequenceProgress = FMath::Min(WinSequenceProgress + DeltaTime / FloatDuration, 1.0f);

	// Smooth easing - ease in-out (slow start, slow end)
	float EasedProgress;
	if (WinSequenceProgress < 0.5f)
	{
		// Ease in (slow start)
		EasedProgress = 4.0f * WinSequenceProgress * WinSequenceProgress * WinSequenceProgress;
	}
	else
	{
		// Ease out (slow end)
		EasedProgress = 1.0f - FMath::Pow(-2.0f * WinSequenceProgress + 2.0f, 3.0f) / 2.0f;
	}

	// Smoothly interpolate relative position
	FVector NewRelativePos = FMath::Lerp(WinMaskMeshStartPos, WinMaskMeshTargetPos, EasedProgress);

	// Add eerie floating motion (decreases as it gets closer)
	float FloatIntensity = 1.0f - EasedProgress * 0.7f;  // Still some float at the end
	float FloatOffset = FMath::Sin(WinSequenceTimer * 1.5f) * 10.0f * FloatIntensity;
	float SideFloat = FMath::Sin(WinSequenceTimer * 0.9f) * 6.0f * FloatIntensity;
	NewRelativePos.Z += FloatOffset;
	NewRelativePos.Y += SideFloat;

	Enemy->Mesh->SetRelativeLocation(NewRelativePos);

	// Smoothly interpolate rotation (from start yaw to target yaw)
	WinMaskRotationProgress = EasedProgress;
	FRotator NewRot = FMath::Lerp(WinMaskMeshStartRot, WinMaskMeshTargetRot, EasedProgress);

	// Add creepy wobble that decreases as it gets closer
	float WobbleIntensity = 1.0f - EasedProgress * 0.8f;
	NewRot.Pitch += FMath::Sin(WinSequenceTimer * 2.0f) * 8.0f * WobbleIntensity;
	NewRot.Roll += FMath::Sin(WinSequenceTimer * 1.7f) * 6.0f * WobbleIntensity;

	Enemy->Mesh->SetRelativeRotation(NewRot);

	// Scale up slightly as it approaches (subtle)
	float BaseScale = 0.25f;  // Original scale from your data
	float ScaleMultiplier = BaseScale * (1.0f + EasedProgress * 0.3f);
	Enemy->Mesh->SetRelativeScale3D(FVector(ScaleMultiplier));

	// === FADE TO BLACK SIMULTANEOUSLY ===
	// Start fading at 70% progress (mask is close to camera), finish by 100%
	float FadeStart = 0.7f;
	if (WinSequenceProgress >= FadeStart)
	{
		// Add widget to viewport when fade actually starts (first time only)
		if (FadeWidgetInstance && !FadeWidgetInstance->IsInViewport())
		{
			FadeWidgetInstance->AddToViewport(100);
			UE_LOG(LogTemp, Warning, TEXT("WIN: Adding fade widget to viewport now"));
		}

		float FadeProgress = (WinSequenceProgress - FadeStart) / (1.0f - FadeStart);
		// Ease the fade - smooth
		float EasedFade = FadeProgress * FadeProgress;  // Ease in (slow start, fast end)
		WinFadeAlpha = FMath::Min(EasedFade, 1.0f);

		// Update the black image widget opacity
		if (BlackImageWidget)
		{
			BlackImageWidget->SetRenderOpacity(WinFadeAlpha);
			BlackImageWidget->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, WinFadeAlpha));
		}
	}
}

void ADiceGameManager::UpdateScreenFade(float DeltaTime)
{
	float FadeSpeed = 1.5f;  // Time to fully fade
	WinFadeAlpha = FMath::Min(WinFadeAlpha + DeltaTime / FadeSpeed, 1.0f);

	// Update the black image widget opacity
	if (BlackImageWidget)
	{
		FLinearColor FadeColor = FLinearColor(0.0f, 0.0f, 0.0f, WinFadeAlpha);
		BlackImageWidget->SetColorAndOpacity(FadeColor);
	}

	// Update timer text during fade
	URoundTimerComponent* Timer = GetRoundTimer();
	if (Timer)
	{
		// Find the text component in the timer's owner
		AActor* TimerOwner = Timer->GetOwner();
		if (TimerOwner)
		{
			UTextRenderComponent* TimerText = TimerOwner->FindComponentByClass<UTextRenderComponent>();
			if (TimerText)
			{
				// Pulse the "VICTORY" text
				float Pulse = (FMath::Sin(WinSequenceTimer * 4.0f) + 1.0f) * 0.5f;
				uint8 Green = static_cast<uint8>(180 + Pulse * 75);
				TimerText->SetText(FText::FromString(TEXT("VICTORY")));
				TimerText->SetTextRenderColor(FColor(100, Green, 100, 255));
			}
		}
	}
}

void ADiceGameManager::CreateFadeWidget()
{
	if (!FadeWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("FadeWidgetClass not set - screen fade will not work"));
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// Create the widget but DON'T add to viewport yet
	FadeWidgetInstance = CreateWidget<UUserWidget>(PC, FadeWidgetClass);
	if (FadeWidgetInstance)
	{
		// Find the BlackImage component - try both possible names
		BlackImageWidget = Cast<UImage>(FadeWidgetInstance->GetWidgetFromName(TEXT("BlackImage")));
		if (!BlackImageWidget)
		{
			// Try alternate name
			BlackImageWidget = Cast<UImage>(FadeWidgetInstance->GetWidgetFromName(TEXT("ImageBlack")));
		}

		if (BlackImageWidget)
		{
			// Start fully transparent
			BlackImageWidget->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
			BlackImageWidget->SetRenderOpacity(0.0f);
			UE_LOG(LogTemp, Warning, TEXT("Fade widget created successfully (not added to viewport yet)"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("BlackImage/ImageBlack not found in fade widget! Check widget design."));
		}
	}
}

void ADiceGameManager::OnWinSequenceComplete()
{
	UE_LOG(LogTemp, Warning, TEXT("WIN SEQUENCE COMPLETE! Player has claimed the mask."));

	// Hide the mask
	AMaskEnemy* Enemy = FindEnemy();
	if (Enemy)
	{
		Enemy->SetActorHiddenInGame(true);
	}

	// Final state - could restart game or show end screen
}

void ADiceGameManager::HideMasqueradeUI()
{
	bTypewriterActive = false;
	bTypewriterOut = false;

	if (MasqueradeUIActor)
	{
		MasqueradeUIActor->SetActorHiddenInGame(true);
	}

	if (MasqueradeUIText)
	{
		MasqueradeUIText->SetText(FText::FromString(TEXT("")));
	}
}

// ==================== DICE LABEL TYPEWRITER ====================

void ADiceGameManager::StartDiceLabelTypewriter()
{
	if (!DiceLabelActor)
	{
		return;
	}

	// Find the text render component if not cached
	if (!DiceLabelTextComp)
	{
		DiceLabelTextComp = DiceLabelActor->FindComponentByClass<UTextRenderComponent>();
	}

	if (!DiceLabelTextComp)
	{
		return;
	}

	// Use the configurable label text
	DiceLabelFullText = DiceLabelText;
	DiceLabelCurrentText = TEXT("");
	DiceLabelTypeTimer = 0.0f;
	DiceLabelTypeIndex = 0;
	bDiceLabelTypewriterActive = true;
	bDiceLabelTypewriterOut = false;  // Typing IN

	// Show the actor and clear the text
	DiceLabelActor->SetActorHiddenInGame(false);
	DiceLabelTextComp->SetText(FText::FromString(TEXT("")));

	UE_LOG(LogTemp, Warning, TEXT("Starting DiceLabel typewriter IN: %s"), *DiceLabelFullText);
}

void ADiceGameManager::StartDiceLabelTypewriterOut()
{
	if (!DiceLabelActor || !DiceLabelTextComp)
	{
		return;
	}

	// If already typing out or not active, just hide
	if (bDiceLabelTypewriterOut || DiceLabelCurrentText.Len() == 0)
	{
		HideDiceLabel();
		return;
	}

	// Start typing OUT (reverse)
	DiceLabelTypeIndex = DiceLabelCurrentText.Len();
	DiceLabelTypeTimer = 0.0f;
	bDiceLabelTypewriterOut = true;
	bDiceLabelTypewriterActive = true;

	UE_LOG(LogTemp, Warning, TEXT("Starting DiceLabel typewriter OUT"));
}

void ADiceGameManager::UpdateDiceLabelTypewriter(float DeltaTime)
{
	if (!bDiceLabelTypewriterActive || !DiceLabelTextComp)
	{
		return;
	}

	DiceLabelTypeTimer += DeltaTime;

	float CharTime = 1.0f / DiceLabelTypeSpeed;

	if (bDiceLabelTypewriterOut)
	{
		// Typing OUT - remove characters
		if (DiceLabelTypeTimer >= CharTime)
		{
			DiceLabelTypeTimer = 0.0f;

			if (DiceLabelTypeIndex > 0)
			{
				DiceLabelTypeIndex--;
				DiceLabelCurrentText = DiceLabelFullText.Left(DiceLabelTypeIndex);
				DiceLabelTextComp->SetText(FText::FromString(DiceLabelCurrentText));
			}
			else
			{
				// Done typing out
				HideDiceLabel();
			}
		}
	}
	else
	{
		// Typing IN - add characters
		if (DiceLabelTypeTimer >= CharTime)
		{
			DiceLabelTypeTimer = 0.0f;

			if (DiceLabelTypeIndex < DiceLabelFullText.Len())
			{
				DiceLabelTypeIndex++;
				DiceLabelCurrentText = DiceLabelFullText.Left(DiceLabelTypeIndex);
				DiceLabelTextComp->SetText(FText::FromString(DiceLabelCurrentText));
			}
			else
			{
				// Done typing in - keep visible
				bDiceLabelTypewriterActive = false;
			}
		}
	}
}

void ADiceGameManager::HideDiceLabel()
{
	bDiceLabelTypewriterActive = false;
	bDiceLabelTypewriterOut = false;

	if (DiceLabelActor)
	{
		DiceLabelActor->SetActorHiddenInGame(true);
	}

	if (DiceLabelTextComp)
	{
		DiceLabelTextComp->SetText(FText::FromString(TEXT("")));
	}
}

// ==================== LOSE SEQUENCE ====================

void ADiceGameManager::StartLoseSequence()
{
	LoseSequencePhase = 1;  // Start with breathing + pan down
	LoseSequenceTimer = 0.0f;
	LoseSequenceProgress = 0.0f;
	LoseFadeAlpha = 0.0f;

	// Store camera start position
	ADiceCamera* Cam = FindCamera();
	if (Cam)
	{
		LoseCameraStartPos = Cam->GetActorLocation();
		LoseCameraStartRot = Cam->GetActorRotation();

		// Move camera forward if offset is set
		if (LoseCameraForwardOffset != 0.0f)
		{
			FVector ForwardOffset = Cam->GetActorForwardVector() * LoseCameraForwardOffset;
			LoseCameraStartPos += ForwardOffset;
			Cam->SetActorLocation(LoseCameraStartPos);
		}

		// Target rotation - look down from where we are
		LoseCameraTargetRot = LoseCameraStartRot;
		LoseCameraTargetRot.Pitch = -70.0f;  // Look down

		bLoseCameraBreathing = true;
		LoseCameraBreathTimer = 0.0f;
	}

	// Clear all dice immediately
	for (ADice* Dice : EnemyDice)
	{
		if (Dice) Dice->Destroy();
	}
	EnemyDice.Empty();
	for (ADice* Dice : PlayerDice)
	{
		if (Dice) Dice->Destroy();
	}
	PlayerDice.Empty();

	// Clear arrays
	EnemyResults.Empty();
	PlayerResults.Empty();
	PlayerDiceMatched.Empty();
	EnemyDiceMatched.Empty();

	// Stop any dragging
	bIsDragging = false;
	DraggedDice = nullptr;
	DraggedDiceIndex = -1;

	// Hide modifiers
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod) Mod->SetHidden(true);
	}

	// Create fade widget (starts invisible)
	CreateFadeWidget();

	UE_LOG(LogTemp, Warning, TEXT("LOSE SEQUENCE STARTED!"));
}

void ADiceGameManager::UpdateLoseSequence(float DeltaTime)
{
	if (LoseSequencePhase == 0) return;

	LoseSequenceTimer += DeltaTime;

	// Breathing effect during lose sequence (shaky, panicked)
	if (bLoseCameraBreathing)
	{
		ADiceCamera* Cam = FindCamera();
		if (Cam)
		{
			LoseCameraBreathTimer += DeltaTime;

			// Fast, panicked breathing
			float BreathSpeed = 5.0f;
			float BreathIntensity = 6.0f;

			float Breath1 = FMath::Sin(LoseCameraBreathTimer * BreathSpeed) * BreathIntensity;
			float Breath2 = FMath::Sin(LoseCameraBreathTimer * BreathSpeed * 1.5f) * (BreathIntensity * 0.3f);

			FVector BreathOffset = FVector(0, 0, Breath1 + Breath2);
			Cam->SetActorLocation(LoseCameraStartPos + BreathOffset);
		}
	}

	switch (LoseSequencePhase)
	{
		case 1:  // Breathing + camera pan down
		{
			float PanDuration = 2.0f;
			LoseSequenceProgress = FMath::Min(LoseSequenceProgress + DeltaTime / PanDuration, 1.0f);

			// Ease out for smooth pan
			float EasedProgress = 1.0f - FMath::Pow(1.0f - LoseSequenceProgress, 3.0f);

			ADiceCamera* Cam = FindCamera();
			if (Cam)
			{
				FRotator NewRot = FMath::Lerp(LoseCameraStartRot, LoseCameraTargetRot, EasedProgress);
				// Add breathing shake
				NewRot.Pitch += FMath::Sin(LoseCameraBreathTimer * 5.0f) * 2.0f;
				NewRot.Roll += FMath::Sin(LoseCameraBreathTimer * 4.0f) * 1.0f;
				Cam->SetActorRotation(NewRot);
			}

			if (LoseSequenceProgress >= 1.0f)
			{
				LoseSequencePhase = 2;
				LoseSequenceTimer = 0.0f;
				LoseSequenceProgress = 0.0f;

				// Spawn and drop player mask + knife
				SpawnAndDropPlayerMask();
				DropLastKnife();
			}
			break;
		}

		case 2:  // Mask drop + knife drop + start fade
		{
			float DropDuration = 2.5f;
			LoseSequenceProgress = FMath::Min(LoseSequenceProgress + DeltaTime / DropDuration, 1.0f);

			// Start fading at 30%
			if (LoseSequenceProgress >= 0.3f)
			{
				float FadeProgress = (LoseSequenceProgress - 0.3f) / 0.7f;
				LoseFadeAlpha = FMath::Min(FadeProgress * FadeProgress, 1.0f);

				// Add widget to viewport when fade starts
				if (FadeWidgetInstance && !FadeWidgetInstance->IsInViewport())
				{
					FadeWidgetInstance->AddToViewport(100);
				}

				if (BlackImageWidget)
				{
					BlackImageWidget->SetRenderOpacity(LoseFadeAlpha);
					BlackImageWidget->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, LoseFadeAlpha));
				}
			}

			if (LoseSequenceProgress >= 1.0f && LoseFadeAlpha >= 0.95f)
			{
				LoseSequencePhase = 3;
				LoseSequenceTimer = 0.0f;
				bLoseCameraBreathing = false;
				OnLoseSequenceComplete();
			}
			break;
		}

		case 3:  // Done
		{
			// Could add "Press any key to restart" logic here
			break;
		}
	}
}

void ADiceGameManager::SpawnAndDropPlayerMask()
{
	if (!PlayerMaskMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("LOSE: PlayerMaskMesh not set!"));
		return;
	}

	ADiceCamera* Cam = FindCamera();
	if (!Cam) return;

	// Create a new actor to hold the dropping mask
	FVector SpawnPos = Cam->GetActorLocation() + Cam->GetActorForwardVector() * 50.0f;
	SpawnPos.Z += 30.0f;  // Start above camera view
	PlayerMaskDropStartPos = SpawnPos;

	// Use configurable rotation + camera yaw
	FRotator FacingRot = PlayerMaskDropRotation;
	FacingRot.Yaw += Cam->GetActorRotation().Yaw;

	// Create static mesh component dynamically
	AActor* MaskActor = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), SpawnPos, FacingRot);
	if (MaskActor)
	{
		DroppedPlayerMask = NewObject<UStaticMeshComponent>(MaskActor);
		if (DroppedPlayerMask)
		{
			DroppedPlayerMask->SetStaticMesh(PlayerMaskMesh);
			if (PlayerMaskMaterial)
			{
				DroppedPlayerMask->SetMaterial(0, PlayerMaskMaterial);
			}
			DroppedPlayerMask->RegisterComponent();
			DroppedPlayerMask->AttachToComponent(MaskActor->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			MaskActor->SetRootComponent(DroppedPlayerMask);

			DroppedPlayerMask->SetWorldLocation(SpawnPos);
			DroppedPlayerMask->SetWorldRotation(FacingRot);
			DroppedPlayerMask->SetWorldScale3D(FVector(PlayerMaskDropScale));

			// Enable physics for rigid body drop
			DroppedPlayerMask->SetSimulatePhysics(true);
			DroppedPlayerMask->SetEnableGravity(true);
			DroppedPlayerMask->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			DroppedPlayerMask->SetCollisionResponseToAllChannels(ECR_Block);

			// Add gentle tumble as it falls
			DroppedPlayerMask->AddAngularImpulseInDegrees(FVector(
				FMath::RandRange(-20.0f, 20.0f),
				FMath::RandRange(-20.0f, 20.0f),
				FMath::RandRange(-10.0f, 10.0f)
			));

			UE_LOG(LogTemp, Warning, TEXT("LOSE: Player mask spawned and dropping"));
		}
	}
}

void ADiceGameManager::DropLastKnife()
{
	// Get the player hand to find the knife
	UPlayerHandComponent* PlayerHand = GetPlayerHand();
	if (!PlayerHand)
	{
		UE_LOG(LogTemp, Warning, TEXT("LOSE: No player hand for knife drop"));
		return;
	}

	// Find the knife mesh in the hand actor
	AActor* HandActor = PlayerHand->GetOwner();
	if (!HandActor) return;

	// Look for a static mesh component that might be the knife
	TArray<UStaticMeshComponent*> MeshComps;
	HandActor->GetComponents<UStaticMeshComponent>(MeshComps);

	for (UStaticMeshComponent* Mesh : MeshComps)
	{
		if (Mesh && Mesh->GetName().Contains(TEXT("Knife")))
		{
			// Found the knife - enable physics
			Mesh->SetSimulatePhysics(true);
			Mesh->SetEnableGravity(true);
			Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Mesh->SetCollisionResponseToAllChannels(ECR_Block);

			// Add impulse to make it tumble
			Mesh->AddImpulse(FVector(0, 0, -100.0f));
			Mesh->AddAngularImpulseInDegrees(FVector(
				FMath::RandRange(-100.0f, 100.0f),
				FMath::RandRange(-100.0f, 100.0f),
				FMath::RandRange(-50.0f, 50.0f)
			));

			DroppedKnife = Mesh;
			UE_LOG(LogTemp, Warning, TEXT("LOSE: Knife dropping"));
			break;
		}
	}
}

void ADiceGameManager::OnLoseSequenceComplete()
{
	UE_LOG(LogTemp, Warning, TEXT("LOSE SEQUENCE COMPLETE! Game Over."));

	// Clean up dropped mask
	if (DroppedPlayerMask)
	{
		AActor* MaskOwner = DroppedPlayerMask->GetOwner();
		if (MaskOwner)
		{
			MaskOwner->Destroy();
		}
		DroppedPlayerMask = nullptr;
	}
}
