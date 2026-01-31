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

	// Camera
	CameraPanProgress = 0.0f;
	bCameraPanning = false;
	bCameraAtMatchView = false;
	OriginalCameraLocation = FVector::ZeroVector;
	OriginalCameraRotation = FRotator::ZeroRotator;
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
			InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ADiceGameManager::OnToggleFaceRotationMode);
			InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ADiceGameManager::OnMousePressed);
			InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ADiceGameManager::OnMouseReleased);
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
			UpdateCameraPan(DeltaTime);
			break;

		case EGamePhase::RoundEnd:
		case EGamePhase::GameOver:
			UpdateCameraPan(DeltaTime);
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
			if (Mod) Mod->bIsUsed = false;
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
	ActivateModifiers();
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
		PlayerDiceMatched[PlayerIndex] = true;
		EnemyDiceMatched[EnemyIndex] = true;

		PlayerDice[PlayerIndex]->SetMatched(true);
		EnemyDice[EnemyIndex]->SetMatched(true);

		// Play match effect
		FVector MatchPos = (PlayerDice[PlayerIndex]->GetActorLocation() + EnemyDice[EnemyIndex]->GetActorLocation()) / 2.0f;
		PlayMatchEffect(MatchPos);

		CheckAllMatched();
	}
}

void ADiceGameManager::TryApplyModifier(ADiceModifier* Modifier, int32 DiceIndex)
{
	if (!Modifier || Modifier->bIsUsed) return;
	if (!PlayerDice.IsValidIndex(DiceIndex)) return;
	if (PlayerDiceMatched[DiceIndex]) return;
	if (PlayerDiceModified[DiceIndex]) return;  // Already used a modifier

	// Handle special reroll modifiers
	if (Modifier->ModifierType == EModifierType::RerollOne)
	{
		// RE:1 - Throw this single die again
		RerollSingleDice(DiceIndex);
		Modifier->UseModifier();
		return;
	}
	else if (Modifier->ModifierType == EModifierType::RerollAll)
	{
		// RE:ALL - Throw all unmatched dice again
		RerollAllUnmatchedDice();
		Modifier->UseModifier();
		return;
	}

	// Apply value-changing modifiers (+1, -1, +2, FLIP)
	int32 OldValue = PlayerResults[DiceIndex];
	int32 NewValue = Modifier->ApplyToValue(OldValue);

	// Update the result
	PlayerResults[DiceIndex] = NewValue;

	// Update the dice visual
	if (PlayerDice[DiceIndex])
	{
		PlayerDice[DiceIndex]->CurrentValue = NewValue;
		UpdateDiceFaceDisplay(PlayerDice[DiceIndex], NewValue);
	}

	// Mark as modified and snap to modifier
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

	// Lerp position and rotation
	FVector NewPos = FMath::Lerp(ReturnStartPos, ReturnTargetPos, SmoothAlpha);
	FRotator NewRot = FMath::Lerp(ReturnStartRot, ReturnTargetRot, SmoothAlpha);

	Dice->SetActorLocation(NewPos);
	Dice->SetActorRotation(NewRot);

	if (ReturnProgress >= 1.0f)
	{
		Dice->SetActorLocation(ReturnTargetPos);
		Dice->SetActorRotation(ReturnTargetRot);
		bDiceSnappingToModifier = false;
		SnapTargetModifier = nullptr;
		SnapDiceIndex = -1;
	}
}

void ADiceGameManager::RerollSingleDice(int32 DiceIndex)
{
	if (!PlayerDice.IsValidIndex(DiceIndex)) return;

	ADice* Dice = PlayerDice[DiceIndex];
	if (!Dice) return;

	// Re-enable physics and throw
	Dice->Mesh->SetSimulatePhysics(true);
	Dice->bHasBeenThrown = false;

	// Throw upward with random direction
	FVector ThrowDir = FVector(
		FMath::RandRange(-0.3f, 0.3f),
		FMath::RandRange(-0.3f, 0.3f),
		1.0f
	).GetSafeNormal();

	Dice->Throw(ThrowDir, DiceThrowForce * 0.6f);

	// Mark that we need to wait for this dice to settle and update its value
	bPlayerDiceSettled = false;
	WaitTimer = 0.0f;

	// Temporarily go back to settling phase to wait for the dice
	CurrentPhase = EGamePhase::PlayerDiceSettling;
}

void ADiceGameManager::RerollAllUnmatchedDice()
{
	bool bAnyRerolled = false;

	for (int32 i = 0; i < PlayerDice.Num(); i++)
	{
		if (PlayerDiceMatched[i] || PlayerDiceModified[i]) continue;

		ADice* Dice = PlayerDice[i];
		if (!Dice) continue;

		// Re-enable physics and throw
		Dice->Mesh->SetSimulatePhysics(true);
		Dice->bHasBeenThrown = false;

		// Throw upward with random direction
		FVector ThrowDir = FVector(
			FMath::RandRange(-0.3f, 0.3f),
			FMath::RandRange(-0.3f, 0.3f),
			1.0f
		).GetSafeNormal();

		Dice->Throw(ThrowDir, DiceThrowForce * 0.6f);
		bAnyRerolled = true;
	}

	if (bAnyRerolled)
	{
		// Go back to settling phase to wait for dice
		bPlayerDiceSettled = false;
		WaitTimer = 0.0f;
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
		DealDamage(true);
		CheckGameOver();

		DeactivateModifiers();
		ResetCamera();

		if (CurrentPhase != EGamePhase::GameOver)
		{
			CurrentPhase = EGamePhase::RoundEnd;
		}
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
			Text = bIsDragging ? "Drop on matching dice!" : "Drag your dice!";
			Color = FColor::Yellow;
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
	FString HealthText = FString::Printf(TEXT("Enemy HP: %d/%d | Your HP: %d/%d"), EnemyHealth, MaxHealth, PlayerHealth, MaxHealth);
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
	if (bDiceReturning || bDiceSnappingToModifier) return;

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

	FVector HitLocation;
	AActor* HitActor = GetActorUnderMouse(HitLocation);

	bool bSuccess = false;
	bool bAppliedModifier = false;

	if (HitActor)
	{
		// Check if dropped on enemy dice for matching
		ADice* HitDice = Cast<ADice>(HitActor);
		if (HitDice)
		{
			for (int32 i = 0; i < EnemyDice.Num(); i++)
			{
				if (EnemyDice[i] == HitDice && !EnemyDiceMatched[i])
				{
					TryMatchDice(DraggedDiceIndex, i);
					bSuccess = PlayerDiceMatched[DraggedDiceIndex];
					break;
				}
			}
		}

		// Check if dropped on modifier (only if not already modified)
		ADiceModifier* HitMod = Cast<ADiceModifier>(HitActor);
		if (HitMod && !HitMod->bIsUsed && HitMod->bIsActive)
		{
			// Check if this dice can use modifiers
			if (!PlayerDiceModified[DraggedDiceIndex])
			{
				TryApplyModifier(HitMod, DraggedDiceIndex);
				bAppliedModifier = true;
				bSuccess = true;
			}
		}
	}

	// If applied modifier, don't return dice to original position (it's snapping to modifier)
	if (bAppliedModifier)
	{
		DraggedDice->SetHighlighted(false);
		ClearAllHighlights();
		bIsDragging = false;
		DraggedDice = nullptr;
		DraggedDiceIndex = -1;
	}
	else
	{
		StopDragging(bSuccess);
	}
}

void ADiceGameManager::UpdateMouseInput()
{
	if (bIsDragging)
	{
		UpdateDragging();
		HighlightValidTargets();
	}
	else if (!bDiceReturning && !bDiceSnappingToModifier)
	{
		FVector HitLocation;
		AActor* HitActor = GetActorUnderMouse(HitLocation);

		ClearAllHighlights();

		if (HitActor)
		{
			ADice* HitDice = Cast<ADice>(HitActor);
			if (HitDice)
			{
				for (int32 i = 0; i < PlayerDice.Num(); i++)
				{
					if (PlayerDice[i] == HitDice && !PlayerDiceMatched[i])
					{
						HitDice->SetHighlighted(true);
						break;
					}
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

	// If dice is at a modifier, return position is at the modifier
	if (PlayerDiceModified.IsValidIndex(Index) && PlayerDiceModified[Index] && PlayerDiceAtModifier[Index])
	{
		OriginalDragPosition = PlayerDiceAtModifier[Index]->GetActorLocation() + FVector(0, 0, 20.0f);
	}
	else
	{
		OriginalDragPosition = Dice->GetActorLocation();
	}

	// Store actual current rotation - don't recalculate to avoid flipping
	OriginalDragRotation = Dice->GetActorRotation();

	LastDragPosition = Dice->GetActorLocation();
	Dice->SetHighlighted(true);
}

void ADiceGameManager::StopDragging(bool bSuccess)
{
	if (!DraggedDice)
	{
		bIsDragging = false;
		return;
	}

	DraggedDice->SetHighlighted(false);
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

	ReturnProgress += DeltaTime * 4.0f;
	float Alpha = FMath::Clamp(ReturnProgress, 0.0f, 1.0f);

	// Elastic ease for juicy bounce
	float PosAlpha = EaseOutElastic(Alpha);
	float RotAlpha = EaseOutCubic(FMath::Clamp(Alpha * 1.5f, 0.0f, 1.0f));

	// Position with arc
	FVector NewPos = FMath::Lerp(ReturnStartPos, ReturnTargetPos, PosAlpha);

	// Add arc height
	float ArcHeight = FMath::Sin(Alpha * PI) * 15.0f;
	NewPos.Z += ArcHeight;

	// Add initial velocity influence (fades out)
	float VelInfluence = FMath::Max(0.0f, 0.3f - Alpha * 0.5f);
	NewPos += ReturnVelocity * VelInfluence * DeltaTime;

	ReturningDice->SetActorLocation(NewPos);

	// Rotation with wobble
	FRotator NewRot = FMath::Lerp(ReturnStartRot, ReturnTargetRot, RotAlpha);
	float Wobble = FMath::Sin(Alpha * PI * 5.0f) * (1.0f - Alpha) * 12.0f;
	NewRot.Roll += Wobble;
	NewRot.Pitch += Wobble * 0.7f;
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

	// Only highlight modifiers if this dice hasn't been modified yet
	bool bCanUseModifiers = !PlayerDiceModified[DraggedDiceIndex];
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && !Mod->bIsUsed && Mod->bIsActive)
		{
			Mod->SetHighlighted(bCanUseModifiers);
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
		if (Mod) Mod->SetHighlighted(false);
	}
}

void ADiceGameManager::PlayMatchEffect(FVector Location)
{
	// Debug effect - replace with particles later
	for (int32 i = 0; i < 12; i++)
	{
		FVector Offset = FVector(
			FMath::RandRange(-25.0f, 25.0f),
			FMath::RandRange(-25.0f, 25.0f),
			FMath::RandRange(0.0f, 40.0f)
		);
		DrawDebugPoint(GetWorld(), Location + Offset, 8.0f, FColor::Green, false, 0.4f);
	}
	DrawDebugSphere(GetWorld(), Location, 20.0f, 8, FColor::Yellow, false, 0.3f);
}

// ==================== MODIFIERS ====================

void ADiceGameManager::ActivateModifiers()
{
	for (ADiceModifier* Mod : AllModifiers)
	{
		if (Mod && !Mod->bIsUsed)
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
