#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MaskEnemy.generated.h"

UCLASS()
class AMaskEnemy : public AActor
{
	GENERATED_BODY()

public:
	AMaskEnemy();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup", meta = (ToolTip = "Drag custom mesh here (leave empty to use default cube)"))
	UStaticMesh* CustomMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup", meta = (ToolTip = "Material for the enemy mesh"))
	UMaterialInterface* CustomMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bob")
	float BobSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bob")
	float BobAmplitude;

private:
	float TimeAccumulator;
	FVector InitialLocation;
};
