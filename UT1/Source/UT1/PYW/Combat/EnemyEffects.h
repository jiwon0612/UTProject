#pragma once

#include "CoreMinimal.h"

class UNiagaraComponent;
class UNiagaraSystem;

namespace EnemyEffects
{
	/**
	 * 월드 위치에 Niagara 이펙트를 한 번 생성함. ProjectileVFX 팩 시스템은 발사·비행·폭발 이미터를
	 * 한 시스템에 담고 있어서, 필요 없는 이미터를 활성화 전에 꺼서 씀.
	 */
	UNiagaraComponent* SpawnAtLocation(const UObject* WorldContext, UNiagaraSystem* System, const FVector& Location,
		const FRotator& Rotation, const FVector& Scale, const TArray<FName>& DisabledEmitters);

	/** 이미 만든(자동 활성화하지 않은) 컴포넌트에서 이미터를 끈 뒤 활성화함 */
	void ActivateWithDisabledEmitters(UNiagaraComponent* Effect, const TArray<FName>& DisabledEmitters);
}
