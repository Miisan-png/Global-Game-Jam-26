#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dice.h"
#include "DiceModifier.h"
#include "IRButtonComponent.h"
#include "DiceGameManager.generated.h"

class AMaskEnemy;
class ADiceCamera;
class UPlayerHandComponent;
class URoundTimerComponent;
class ASoundManager;
class UHangingBoardComponent;
class UUserWidget;
class UImage;
class UIRButtonComponent;

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

	// ===== DICE LABEL (Fold prompt) =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Label", meta = (ToolTip = "TextRender actor for fold prompt - typewriter style"))
	AActor* DiceLabelActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Label")
	FString DiceLabelText;  // e.g. "SPACE TO FOLD"

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dice Label")
	float DiceLabelTypeSpeed;  // Characters per second

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

	// ===== TIMER =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timer", meta = (ToolTip = "Actor with RoundTimerComponent attached"))
	AActor* RoundTimerActor;

	// ===== SOUND =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound", meta = (ToolTip = "SoundManager actor in the scene"))
	ASoundManager* SoundManager;

	// ===== BONUS ROUND =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round", meta = (ToolTip = "Actor with HangingBoardComponent"))
	AActor* HangingBoardActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round", meta = (ToolTip = "YES button actor with IRButtonComponent"))
	AActor* YesButtonActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round", meta = (ToolTip = "NO button actor with IRButtonComponent"))
	AActor* NoButtonActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	bool bEnableBonusRound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	float BonusCameraDistance;  // How far back from buttons (X)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	float BonusCameraSide;  // Left/right offset (Y)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	float BonusCameraHeight;  // How high above buttons (Z)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	float BonusCameraPitch;  // Camera angle (negative = look down)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	float BonusCameraFocusSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	bool bDebugBonusCamera;  // Lock camera to bonus view for testing

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round", meta = (ToolTip = "TextRender actor for Masquerade UI - shows with glitchy typewriter effect"))
	AActor* MasqueradeUIActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	float TypewriterSpeed;  // Characters per second

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	float GlitchChance;  // Chance of glitch per character (0-1)

	// Masked Dice visuals
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	UStaticMesh* MaskedDiceMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	UMaterialInterface* MaskedDiceMaterial;

	// Bonus Round Gameplay
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	FVector BonusDiceSpawnPos;  // Where enemy masked dice spawn

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	FVector BonusPlayerDicePos;  // Where YES/NO choice dice appear

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bonus Round")
	float BonusDiceSpacing;  // Space between dice

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
	void OnDebugKillEnemy();  // X key - damage enemy for testing
	void OnDebugKillPlayer(); // Z key - damage player for testing
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
	ADice* LastHoveredDice;

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

	// Round timer integration
	URoundTimerComponent* GetRoundTimer();
	UFUNCTION()
	void OnRoundTimerExpired();
	URoundTimerComponent* CachedRoundTimer;

	// Bonus round integration
	UHangingBoardComponent* GetHangingBoard();
	UIRButtonComponent* GetYesButton();
	UIRButtonComponent* GetNoButton();
	void StartBonusRoundPrompt();
	UFUNCTION()
	void OnBoardArrived();
	UFUNCTION()
	void OnBonusButtonPressed(EIRButtonType ButtonType);
	UFUNCTION()
	void OnBoardRetracted();

	UHangingBoardComponent* CachedHangingBoard;
	UIRButtonComponent* CachedYesButton;
	UIRButtonComponent* CachedNoButton;
	bool bWaitingForBonusChoice;
	bool bBonusRoundAccepted;

	// Bonus Round Gameplay State
	// Phases: 0=inactive, 1=enemy throw, 2=enemy settle, 3=enemy lineup,
	//         4=player throw, 5=player settle, 6=player lineup, 7=player drag,
	//         8=reveal, 9=result pop
	int32 BonusPhase;

	TArray<ADice*> BonusMaskedDice;    // Enemy's 2 masked dice
	ADice* BonusPlayerDice;            // Player's YES dice to drag
	ADice* BonusRevealDice;            // Shows the total after player chooses

	int32 BonusEnemyTotal;             // Sum of enemy dice (not 7)
	bool bBonusIsHigher;               // True if total > 7
	bool bPlayerGuessedHigher;         // What player chose
	int32 BonusDiceModifier;           // +1 or -1 for next round (temporary)
	bool bBonusWon;                    // Result of bonus round
	bool bBonusActiveThisRound;        // True if we're in the bonus-affected round (reset after)
	bool bBonusRoundJustEnded;         // True right after bonus round ends, cleared on next ContinueToNextRound
	static const int32 BaseDiceCount = 5;  // Default dice count to return to

	float BonusAnimTimer;
	float BonusLineupProgress;
	float BonusPlayerLineupProgress;
	float BonusRevealProgress;

	// Masquerade UI Typewriter
	class UTextRenderComponent* MasqueradeUIText;
	FString MasqueradeFullText;
	FString MasqueradeCurrentText;
	float TypewriterTimer;
	int32 TypewriterIndex;
	bool bTypewriterActive;
	bool bTypewriterOut;  // True = typing out (reverse), False = typing in
	float GlitchTimer;
	bool bShowingGlitch;
	FString GlitchChars;

	void StartMasqueradeTypewriter();
	void StartMasqueradeTypewriterOut();  // Reverse effect
	void UpdateMasqueradeTypewriter(float DeltaTime);
	void HideMasqueradeUI();

	// Dice Label Typewriter (fold prompt)
	class UTextRenderComponent* DiceLabelTextComp;
	FString DiceLabelFullText;
	FString DiceLabelCurrentText;
	float DiceLabelTypeTimer;
	int32 DiceLabelTypeIndex;
	bool bDiceLabelTypewriterActive;
	bool bDiceLabelTypewriterOut;

	void StartDiceLabelTypewriter();
	void StartDiceLabelTypewriterOut();
	void UpdateDiceLabelTypewriter(float DeltaTime);
	void HideDiceLabel();

	TArray<FVector> BonusDiceStartPositions;
	TArray<FRotator> BonusDiceStartRotations;
	TArray<FVector> BonusDiceTargetPositions;
	TArray<FRotator> BonusDiceTargetRotations;

	FVector BonusPlayerDiceStartPos;
	FRotator BonusPlayerDiceStartRot;
	FVector BonusPlayerDiceTargetPos;
	FRotator BonusPlayerDiceTargetRot;

	void StartBonusRoundGame();
	void ThrowBonusMaskedDice();
	void CheckBonusDiceSettled();
	void PrepareBonusDiceLineup();
	void LineUpBonusDice(float DeltaTime);
	void ThrowBonusPlayerDice();
	void CheckBonusPlayerDiceSettled();
	void PrepareBonusPlayerLineup();
	void LineUpBonusPlayerDice(float DeltaTime);
	void StartBonusPlayerTurn();
	void OnBonusModifierSelected(bool bHigher);
	void StartBonusRevealSequence();
	void UpdateBonusReveal(float DeltaTime);
	void ShowBonusResult();
	void EndBonusRound(bool bWon);
	void UpdateBonusRound(float DeltaTime);
	void CleanupBonusRound();
	int32 GenerateBonusTotal();

	// Bonus reveal animation states
	// RevealPhase: 0=shake, 1=strike incoming, 2=impact/fly away, 3=result slide, 4=done
	int32 BonusRevealPhase;
	float RevealShakeTimer;
	float RevealStrikeProgress;
	FVector RevealDiceStartPos;
	FVector RevealDiceTargetPos;
	TArray<FVector> MaskedDicePreShakePos;
	TArray<FVector> MaskedDiceFlyVelocity;

	// Bonus dice snap animation (for player dice placement)
	bool bBonusDiceSnapping;
	float BonusSnapProgress;
	FVector BonusSnapStartPos;
	FRotator BonusSnapStartRot;
	FVector BonusSnapTargetPos;
	FRotator BonusSnapTargetRot;
	bool bBonusSnapChoseHigher;  // Store choice during snap animation
	ADiceModifier* SelectedBonusModifier;  // Track which modifier was chosen
	void StartBonusDiceSnap(ADiceModifier* Modifier, bool bChoseHigher);
	void UpdateBonusDiceSnap(float DeltaTime);

	// Bonus result animation state (physics-based arc)
	FVector BonusRevealDiceStartPos;
	FRotator BonusRevealDiceStartRot;
	FVector BonusResultTargetPos;  // Where both dice bounce to
	FVector BonusPlayerVelocity;   // Physics velocity for player dice
	FVector BonusRevealVelocity;   // Physics velocity for reveal dice
	float BonusPlayerRotSpeed;     // Rotation speed
	float BonusRevealRotSpeed;
	int32 BonusBounceCount;        // Track bounces for damping
	float BonusResultFloorZ;       // Floor level for bouncing

	// Bonus camera shake
	bool bBonusCameraShaking;
	float BonusCameraShakeTimer;
	float BonusCameraShakeDuration;
	float BonusCameraShakeIntensity;
	FVector BonusCameraShakeOffset;
	void StartBonusCameraShake(float Duration, float Intensity);
	void UpdateBonusCameraShake(float DeltaTime);

	// Bonus round camera
	void StartBonusButtonCameraFocus();
	void StartBonusButtonCameraReturn();
	void UpdateBonusCameraFocus(float DeltaTime);
	FVector BonusCameraTargetPos;
	FRotator BonusCameraTargetRot;
	FVector BonusCameraOriginalPos;
	FRotator BonusCameraOriginalRot;
	float BonusCameraProgress;
	bool bBonusCameraFocusing;
	bool bBonusCameraReturning;

	// ===== WIN SEQUENCE =====
	// Phases: 0=inactive, 1=modifiers fade, 2=mask float toward camera + fade, 3=done
	int32 WinSequencePhase;
	float WinSequenceTimer;
	float WinSequenceProgress;

	// Mask mesh animation (move mesh component, not actor)
	FVector WinMaskMeshStartPos;      // Mesh component's starting relative position
	FRotator WinMaskMeshStartRot;     // Mesh component's starting relative rotation
	FVector WinMaskMeshTargetPos;     // Target RELATIVE position (close to player)
	FRotator WinMaskMeshTargetRot;    // Target RELATIVE rotation
	float WinMaskRotationProgress;    // For rotation tracking

	// Win camera breathing (faster, more intense)
	bool bWinCameraBreathing;
	float WinCameraBreathTimer;
	FVector WinCameraBasePos;
	FRotator WinCameraBaseRot;

	// Fade to black - UMG Widget
	UPROPERTY(EditAnywhere, Category = "Win Sequence", meta = (ToolTip = "UMG Widget with a UImage named BlackImage for fade effect"))
	TSubclassOf<UUserWidget> FadeWidgetClass;

	UPROPERTY()
	UUserWidget* FadeWidgetInstance;

	UPROPERTY()
	UImage* BlackImageWidget;

	float WinFadeAlpha;

	// Modifier fade
	TArray<float> ModifierFadeAlpha;

	void StartWinSequence();
	void UpdateWinSequence(float DeltaTime);
	void UpdateModifierFade(float DeltaTime);
	void UpdateMaskFloat(float DeltaTime);
	void UpdateScreenFade(float DeltaTime);
	void OnWinSequenceComplete();
	void CreateFadeWidget();

	// ===== LOSE SEQUENCE =====
	// Phases: 0=inactive, 1=breathing+pan down, 2=mask drop+knife drop, 3=fade out, 4=done
	UPROPERTY(EditAnywhere, Category = "Lose Sequence", meta = (ToolTip = "Static mesh for player's mask that drops on lose"))
	UStaticMesh* PlayerMaskMesh;

	UPROPERTY(EditAnywhere, Category = "Lose Sequence")
	UMaterialInterface* PlayerMaskMaterial;

	UPROPERTY(EditAnywhere, Category = "Lose Sequence")
	float PlayerMaskDropScale;

	UPROPERTY(EditAnywhere, Category = "Lose Sequence", meta = (ToolTip = "Rotation offset for dropped mask"))
	FRotator PlayerMaskDropRotation;

	UPROPERTY(EditAnywhere, Category = "Lose Sequence", meta = (ToolTip = "How far camera moves forward before looking down"))
	float LoseCameraForwardOffset;

	int32 LoseSequencePhase;
	float LoseSequenceTimer;
	float LoseSequenceProgress;
	float LoseFadeAlpha;

	// Camera pan down
	FVector LoseCameraStartPos;
	FRotator LoseCameraStartRot;
	FRotator LoseCameraTargetRot;  // Looking down
	bool bLoseCameraBreathing;
	float LoseCameraBreathTimer;

	// Player mask drop (not UPROPERTY - owned by spawned actor)
	UStaticMeshComponent* DroppedPlayerMask;
	FVector PlayerMaskDropStartPos;

	// Knife drop (not UPROPERTY - just a reference)
	UStaticMeshComponent* DroppedKnife;

	void StartLoseSequence();
	void UpdateLoseSequence(float DeltaTime);
	void SpawnAndDropPlayerMask();
	void DropLastKnife();
	void OnLoseSequenceComplete();
};
