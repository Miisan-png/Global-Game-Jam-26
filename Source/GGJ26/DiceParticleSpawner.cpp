#include "DiceParticleSpawner.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"

ADiceParticleSpawner* ADiceParticleSpawner::Instance = nullptr;

ADiceParticleSpawner::ADiceParticleSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	DustSystem = nullptr;
	ImpactSystem = nullptr;
	MatchSystem = nullptr;
	DamageSystem = nullptr;
	ModifierSystem = nullptr;
}

void ADiceParticleSpawner::BeginPlay()
{
	Super::BeginPlay();
	Instance = this;
}

ADiceParticleSpawner* ADiceParticleSpawner::GetInstance(UWorld* World)
{
	if (Instance)
	{
		return Instance;
	}

	if (World)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(World, ADiceParticleSpawner::StaticClass(), Found);
		if (Found.Num() > 0)
		{
			Instance = Cast<ADiceParticleSpawner>(Found[0]);
			return Instance;
		}
	}

	return nullptr;
}

UNiagaraComponent* ADiceParticleSpawner::SpawnSystem(UNiagaraSystem* System, FVector Location, FRotator Rotation)
{
	if (!System)
	{
		return nullptr;
	}

	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		System,
		Location,
		Rotation,
		FVector(1.0f),
		true,
		true,
		ENCPoolMethod::AutoRelease
	);
}

void ADiceParticleSpawner::SpawnParticleAt(FVector Location, EParticleType Type, FLinearColor Color, float Scale)
{
	UNiagaraSystem* SystemToUse = nullptr;

	switch (Type)
	{
		case EParticleType::DiceDust:
			SystemToUse = DustSystem;
			break;
		case EParticleType::DiceImpact:
			SystemToUse = ImpactSystem;
			break;
		case EParticleType::DiceMatch:
			SystemToUse = MatchSystem;
			break;
		case EParticleType::DiceDamage:
			SystemToUse = DamageSystem;
			break;
		case EParticleType::ModifierActivate:
			SystemToUse = ModifierSystem;
			break;
	}

	UNiagaraComponent* Comp = SpawnSystem(SystemToUse, Location);
	if (Comp)
	{
		Comp->SetVariableLinearColor(FName("Color"), Color);
		Comp->SetVariableFloat(FName("Scale"), Scale);
	}
}

void ADiceParticleSpawner::SpawnDustAt(FVector Location, float Intensity)
{
	UNiagaraComponent* Comp = SpawnSystem(DustSystem, Location);
	if (Comp)
	{
		Comp->SetVariableFloat(FName("Intensity"), Intensity);
		Comp->SetVariableLinearColor(FName("Color"), FLinearColor(0.8f, 0.7f, 0.6f, 1.0f));
	}
}

void ADiceParticleSpawner::SpawnImpactAt(FVector Location, FLinearColor Color)
{
	UNiagaraComponent* Comp = SpawnSystem(ImpactSystem, Location);
	if (Comp)
	{
		Comp->SetVariableLinearColor(FName("Color"), Color);
	}
}

void ADiceParticleSpawner::SpawnMatchEffect(FVector Location)
{
	UNiagaraComponent* Comp = SpawnSystem(MatchSystem, Location);
	if (Comp)
	{
		Comp->SetVariableLinearColor(FName("Color"), FLinearColor(0.2f, 1.0f, 0.3f, 1.0f));
	}
}

void ADiceParticleSpawner::SpawnDamageEffect(FVector Location, bool bIsEnemy)
{
	UNiagaraComponent* Comp = SpawnSystem(DamageSystem, Location);
	if (Comp)
	{
		FLinearColor Color = bIsEnemy ? FLinearColor(1.0f, 0.3f, 0.2f, 1.0f) : FLinearColor(0.2f, 0.5f, 1.0f, 1.0f);
		Comp->SetVariableLinearColor(FName("Color"), Color);
	}
}

void ADiceParticleSpawner::SpawnModifierEffect(FVector Location, FLinearColor Color)
{
	UNiagaraComponent* Comp = SpawnSystem(ModifierSystem, Location);
	if (Comp)
	{
		Comp->SetVariableLinearColor(FName("Color"), Color);
	}
}
