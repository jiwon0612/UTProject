// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Animation/AnimNotifyState_UT1AttackTrace.h"
#include "CJW/Player/UT1Player.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	// 노티파이는 메시 컴포넌트만 받는다. 실제 처리 주체는 그 소유 액터다.
	AUT1Player* GetOwningPlayer(USkeletalMeshComponent* MeshComp)
	{
		return MeshComp != nullptr ? Cast<AUT1Player>(MeshComp->GetOwner()) : nullptr;
	}
}

void UAnimNotifyState_UT1AttackTrace::NotifyBegin(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AUT1Player* Player = GetOwningPlayer(MeshComp))
	{
		Player->StartWeaponTrace();
	}
}

void UAnimNotifyState_UT1AttackTrace::NotifyTick(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (AUT1Player* Player = GetOwningPlayer(MeshComp))
	{
		Player->TickWeaponTrace();
	}
}

void UAnimNotifyState_UT1AttackTrace::NotifyEnd(USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AUT1Player* Player = GetOwningPlayer(MeshComp))
	{
		Player->StopWeaponTrace();
	}
}

FString UAnimNotifyState_UT1AttackTrace::GetNotifyName_Implementation() const
{
	return TEXT("UT1 Attack Trace");
}
