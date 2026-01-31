#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dice.h"
#include "DiceModifier.h"
#include "DiceGameManager.generated.h"

class AMaskEnemy;
class ADiceCamera;
class UPlayerHandComponent;

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
	Idle,
	EnemyThrowing,
	EnemyDiceSettling,
	EnemyDiceLining,
	PlayerTurn,
	PlayerThrowing,
	PlayerDiceSettling,
	PlayerDiceLining,
	PlayerMatching,
	RoundEnd,
	GameOver
};

UCLASS()
class ADiceGameManager : public AActor
{
	GENERATED_BODY()

public:
	ADiceGameManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ===== SETUP =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	int32 EnemyNumDice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	int32 PlayerNumDice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	float DiceThrowForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FVector EnemyDiceSpawnOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	AActor* TableActor;

	// ===== DICE VISUALS =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Custom mesh for player dice"))
	UStaticMesh* PlayerDiceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Material for player dice"))
	UMaterialInterface* PlayerDiceMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Custom mesh for enemy dice"))
	UStaticMesh* EnemyDiceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Material for enemy dice"))
	UMaterialInterface* EnemyDiceMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Text color for player dice"))
	FColor PlayerTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Text color for enemy dice"))
	FColor EnemyTextColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Enable glow on player dice"))
	bool bPlayerDiceGlow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Scale for dice (1.0 = native mesh size)"))
	float DiceScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Extra scale for custom mesh (adjust if your mesh is too big/small)"))
	float CustomMeshScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Show 3D text numbers on dice faces (disable if mesh has baked numbers)"))
	bool bShowDiceNumbers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Size of face number text"))
	float DiceTextSize;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Visuals", meta = (ToolTip = "Distance of text from dice center (increase for larger meshes)"))
	float DiceTextOffset;

	// ===== LINEUP =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup", meta = (ToolTip = "Center point for dice lineup. If TableActor is set, this is offset from table."))
	FVector LineupCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup", meta = (ToolTip = "Rotation of lineup in degrees. 0 = dice line up along Y axis."))
	float LineupYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup")
	float DiceLineupSpacing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup", meta = (ToolTip = "Distance of enemy row behind center (always goes backward)"))
	float EnemyRowOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup", meta = (ToolTip = "Distance of player row in front of center (always goes forward)"))
	float PlayerRowOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup")
	float DiceLineupHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lineup")
	float DiceLineupSpeed;

	// ===== CAMERA =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	FVector MatchCameraOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MatchCameraPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraPanSpeed;

	// ===== DRAGGING =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dragging")
	float DragHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dragging")
	float DragFollowSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dragging")
	float DragTiltAmount;

	// ===== DEBUG =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebugGizmos;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bAdjustMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	float AdjustStep;

	// ===== HEALTH =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	int32 PlayerHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	int32 EnemyHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
	int32 MaxHealth;

	// ===== HANDS =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hands")
	AActor* PlayerHandActor;  // Actor with PlayerHandComponent

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hands")
	AActor* EnemyHandActor;  // Actor with PlayerHandComponent (bIsPlayerHand = false)

	// ===== STATE (Read Only) =====
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EGamePhase CurrentPhase;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<ADice*> EnemyDice;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<ADice*> PlayerDice;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<int32> EnemyResults;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	TArray<int32> PlayerResults;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	ADice* SelectedPlayerDice;

	UPROPERTY(BlueprintReadOnly, Category = "State")
	int32 SelectedDiceIndex;

	UPROPERTY(BlueprintReadOnly, Category = "Modifiers")
	TArray<ADiceModifier*> AllModifiers;

	UFUNCTION(BlueprintCallable)
	void StartGame();

	UFUNCTION(BlueprintCallable)
	void PlayerThrowDice();

private:
	void SetupInputBindings();
	void OnStartGamePressed();
	void OnPlayerThrowPressed();
	void OnPlayerActionPressed();  // E key - unified action
	void OnToggleDebugPressed();
	void OnToggleFaceRotationMode();
	void OnSelectNext();
	void OnSelectPrev();
	void OnConfirmSelection();
	void OnCancelSelection();
	void OnGiveUpPressed();
	void UpdateDiceDebugVisibility();

	void EnemyThrowDice();
	void CheckEnemyDiceSettled(float DeltaTime);
	void PrepareEnemyDiceLineup();
	void LineUpEnemyDice(float DeltaTime);
	void StartPlayerTurn();
	void CheckPlayerDiceSettled();
	void PreparePlayerDiceLineup();
	void LineUpPlayerDice(float DeltaTime);

	void StartMatchingPhase();
	void UpdateMatchingPhase();
	void SelectDice(int32 Index);
	void TryMatchDice(int32 PlayerIndex, int32 EnemyIndex);
	void TryApplyModifier(ADiceModifier* Modifier, int32 DiceIndex);
	void UpdateSelectionHighlights();
	void RerollSingleDice(int32 DiceIndex);
	void RerollAllUnmatchedDice();
	void StartDiceLiftForReroll();
	void UpdateDiceLiftForReroll(float DeltaTime);
	void UpdateDiceFaceDisplay(ADice* Dice, int32 NewValue);
	void SnapDiceToModifier(int32 DiceIndex, ADiceModifier* Modifier);
	void CheckAllMatched();
	void DealDamage(bool bToEnemy);
	void CheckGameOver();
	bool CanStillMatch();
	void GiveUpRound();
	void ContinueToNextRound();

	void FindAllModifiers();
	void ClearAllDice();
	void StartDiceDisperse();
	void UpdateDiceDisperse(float DeltaTime);
	void DrawTurnText();
	void DrawHealthBars();

	FRotator GetRotationForFaceUp(int32 FaceValue);
	FVector GetLineupWorldCenter();
	float EaseOutCubic(float t);
	float EaseOutElastic(float t);

	AMaskEnemy* FindEnemy();
	ADiceCamera* FindCamera();

	bool bEnemyDiceSettled;
	bool bPlayerDiceSettled;
	float LineupProgress;
	float PlayerLineupProgress;
	float WaitTimer;
	float StaggerDelay;

	TArray<FVector> EnemyDiceStartPositions;
	TArray<FRotator> EnemyDiceStartRotations;
	TArray<FVector> EnemyDiceTargetPositions;
	TArray<FRotator> EnemyDiceTargetRotations;

	TArray<FVector> PlayerDiceStartPositions;
	TArray<FRotator> PlayerDiceStartRotations;
	TArray<FVector> PlayerDiceTargetPositions;
	TArray<FRotator> PlayerDiceTargetRotations;

	TArray<bool> PlayerDiceMatched;
	TArray<bool> EnemyDiceMatched;
	TArray<bool> PlayerDiceModified;  // Dice that have been modified (can't use more modifiers)
	TArray<ADiceModifier*> PlayerDiceAtModifier;  // Which modifier each dice is at (nullptr if none)
	int32 SelectionMode;
	int32 HoveredEnemyIndex;
	int32 HoveredModifierIndex;

	// Dragging state
	bool bIsDragging;
	ADice* DraggedDice;
	int32 DraggedDiceIndex;
	FVector OriginalDragPosition;
	FRotator OriginalDragRotation;
	FVector LastDragPosition;

	// Return animation
	bool bDiceReturning;
	ADice* ReturningDice;
	int32 ReturningDiceIndex;
	float ReturnProgress;
	FVector ReturnStartPos;
	FRotator ReturnStartRot;
	FVector ReturnTargetPos;
	FRotator ReturnTargetRot;
	FVector ReturnVelocity;

	// Modifier snap animation
	bool bDiceSnappingToModifier;
	ADiceModifier* SnapTargetModifier;
	int32 SnapDiceIndex;

	// Modifier effect animation
	bool bDiceFlipping;
	int32 FlipDiceIndex;
	float FlipProgress;
	FRotator FlipStartRot;
	FRotator FlipTargetRot;

	// Reroll from modifier
	bool bRerollAfterSnap;
	int32 RerollDiceIndex;
	bool bRerollAll;
	bool bWaitingForCameraToRerollAll;
	float RerollAllWaitTimer;
	bool bDiceLiftingForReroll;
	float DiceLiftProgress;
	TArray<FVector> RerollStartPositions;
	TArray<FVector> RerollLiftPositions;

	void OnMousePressed();
	void OnMouseReleased();
	void UpdateMouseInput();
	AActor* GetActorUnderMouse(FVector& HitLocation);
	FVector GetMouseWorldPosition();
	void StartDragging(ADice* Dice, int32 Index);
	void StopDragging(bool bSuccess);
	void PhysicsBounceBack();
	void UpdateDragging();
	void UpdateDiceReturn(float DeltaTime);
	void UpdateModifierSnap(float DeltaTime);
	void UpdateDiceFlip(float DeltaTime);
	void StartDiceFlip(int32 DiceIndex, int32 NewValue);
	void UpdateMatchAnimation(float DeltaTime);
	void StartMatchAnimation(int32 PlayerIdx, int32 EnemyIdx);
	void HighlightValidTargets();
	void ClearAllHighlights();
	void PlayMatchEffect(FVector Location);

	// Match animation (Balatro style)
	bool bMatchAnimating;
	int32 MatchPlayerIndex;
	int32 MatchEnemyIndex;
	float MatchAnimProgress;
	FVector MatchStartPos;
	FVector MatchTargetPos;

	void ActivateModifiers();
	void DeactivateModifiers();
	void ResetModifiersForNewRound();
	void StartModifierShuffle();
	void UpdateModifierShuffle(float DeltaTime);

	// Hand chop integration
	void TriggerPlayerChop();
	void TriggerEnemyChop();
	UFUNCTION()
	void OnPlayerChopComplete();
	UFUNCTION()
	void OnEnemyChopComplete();
	UPlayerHandComponent* GetPlayerHand();
	UPlayerHandComponent* GetEnemyHand();
	bool bWaitingForChop;
	bool bWaitingForCameraThenEnemyChop;  // Wait for camera pan to finish before enemy chop

	// Dice disperse animation
	bool bDiceDispersing;
	float DiceDisperseProgress;
	TArray<ADice*> DispersingDice;
	TArray<FVector> DisperseStartPositions;
	TArray<FVector> DisperseVelocities;
	TArray<float> DisperseStartScales;

	void StartCameraPan();
	void UpdateCameraPan(float DeltaTime);
	void ResetCamera();

	// Adjust mode
	void UpdateAdjustMode();
	void DrawAdjustGizmos();
	void PrintCurrentSettings();
	int32 AdjustSelection; // 0=Center, 1=EnemyRow, 2=PlayerRow, 3=Spacing, 4=Yaw

	// Face rotation tool
	bool bFaceRotationMode;
	int32 CurrentFaceEdit; // 1-6
	TArray<FRotator> FaceRotations;
	void UpdateFaceRotationMode();
	void SpawnTestDice();
	void UpdateTestDice();
	ADice* TestDice;

	FVector OriginalCameraLocation;
	FRotator OriginalCameraRotation;
	float CameraPanProgress;
	bool bCameraPanning;
	bool bCameraAtMatchView;

	// Modifier shuffle/fade animation
	TArray<ADiceModifier*> PermanentlyRemovedModifiers;
	bool bModifierShuffling;
	float ModifierShuffleProgress;
	ADiceModifier* FadingModifier;
	TArray<FVector> ModifierStartPositions;
	TArray<FVector> ModifierTargetPositions;
	float FadingModifierAlpha;
	int32 CurrentRound;
};
