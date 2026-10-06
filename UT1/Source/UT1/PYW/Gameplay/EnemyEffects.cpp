#include "PYW/Gameplay/EnemyEffects.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace EnemyEffects
{
	UNiagaraComponent* SpawnAtLocation(const UObject* WorldContext, UNiagaraSystem* System, const FVector& Location,
		const FRotator& Rotation, const FVector& Scale, const TArray<FName>& DisabledEmitters)
	{
		if (!System || !WorldContext) return nullptr;
		UNiagaraComponent* Effect = UNiagaraFunctionLibrary::SpawnSystemAtLocation(WorldContext, System, Location,
			Rotation, Scale, true, false);
		ActivateWithDisabledEmitters(Effect, DisabledEmitters);
		return Effect;
	}

	void ActivateWithDisabledEmitters(UNiagaraComponent* Effect, const TArray<FName>& DisabledEmitters)
	{
		if (!Effect) return;
		// 컴포넌트가 오버라이드로 저장했다가 활성화할 때 적용하므로 반드시 Activate 전에 꺼야 함
		for (const FName& EmitterName : DisabledEmitters)
		{
			Effect->SetEmitterEnable(EmitterName, false);
		}
		Effect->Activate(true);
	}
}
