#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HangingBoardComponent.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoardArrived);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBoardRetracted);

UENUM(BlueprintType)
enum class EBoardState : uint8
{
	Hidden,
	Descending,
	Visible,
	Ascending
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UHangingBoardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHangingBoardComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	FString ContentLabelName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	FVector HiddenOffset;  // Offset from target when hidden (above)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	FVector TargetPosition;  // Where board stops

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	float DescendSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	float AscendSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	float ShimmerAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	float ShimmerSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	FString BoardText;  // Text to display

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	float TextRevealSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Board")
	float DelayBeforeButton;  // Time to wait after arriving before triggering button

	// State
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EBoardState CurrentState;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBoardArrived OnBoardArrived;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnBoardRetracted OnBoardRetracted;

	// Functions
	UFUNCTION(BlueprintCallable)
	void ShowBoard(const FString& Text = "");

	UFUNCTION(BlueprintCallable)
	void HideBoard();

	UFUNCTION(BlueprintCallable)
	void SetBoardText(const FString& Text);

private:
	UTextRenderComponent* ContentLabel;
	UStaticMeshComponent* BoardMesh;

	FVector HiddenPosition;
	FVector StartPosition;
	float AnimProgress;
	float ShimmerTimer;

	// Text reveal
	FString FullText;
	int32 RevealedChars;
	float TextRevealTimer;
	bool bTextRevealing;

	// Delay before button
	float ButtonDelayTimer;
	bool bWaitingForButtonDelay;

	void UpdateDescend(float DeltaTime);
	void UpdateAscend(float DeltaTime);
	void UpdateShimmer(float DeltaTime);
	void UpdateTextReveal(float DeltaTime);

	float EaseOutBounce(float t);
	float EaseInBack(float t);
};
