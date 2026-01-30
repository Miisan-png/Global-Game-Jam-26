#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "DiceParticleSpawner.generated.h"

UENUM(BlueprintType)
enum class EParticleType : uint8
{
	DiceDust,
	DiceImpact,
	DiceMatch,
	DiceDamage,
	ModifierActivate
};

UCLASS()
class ADiceParticleSpawner : public AActor
{
	GENERATED_BODY()

public:
	ADiceParticleSpawner();

	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
	UNiagaraSystem* DustSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
	UNiagaraSystem* ImpactSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
	UNiagaraSystem* MatchSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
	UNiagaraSystem* DamageSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
	UNiagaraSystem* ModifierSystem;

	UFUNCTION(BlueprintCallable, Category = "Particles")
	void SpawnParticleAt(FVector Location, EParticleType Type, FLinearColor Color = FLinearColor::White, float Scale = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Particles")
	void SpawnDustAt(FVector Location, float Intensity = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Particles")
	void SpawnImpactAt(FVector Location, FLinearColor Color = FLinearColor::White);

	UFUNCTION(BlueprintCallable, Category = "Particles")
	void SpawnMatchEffect(FVector Location);

	UFUNCTION(BlueprintCallable, Category = "Particles")
	void SpawnDamageEffect(FVector Location, bool bIsEnemy);

	UFUNCTION(BlueprintCallable, Category = "Particles")
	void SpawnModifierEffect(FVector Location, FLinearColor Color);

	static ADiceParticleSpawner* GetInstance(UWorld* World);

private:
	static ADiceParticleSpawner* Instance;

	UNiagaraComponent* SpawnSystem(UNiagaraSystem* System, FVector Location, FRotator Rotation = FRotator::ZeroRotator);
};
