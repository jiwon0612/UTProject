// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Combat/UT1CombatFeedbackSubsystem.h"
#include "CJW/Combat/UT1DamageNumber.h"
#include "Engine/World.h"

bool UUT1CombatFeedbackSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UUT1CombatFeedbackSubsystem::ShowDamageNumber(const FVector& Location, float Damage, bool bCritical, bool bVictimIsPlayer)
{
	UWorld* World = GetWorld();
	if (World == nullptr || Damage <= 0.0f)
	{
		return;
	}

	// 한 스윙에 여러 번 맞거나 여러 적을 동시에 맞혀도 숫자가 겹치지 않게 살짝 흩뜨린다.
	const FVector Jitter(FMath::FRandRange(-25.0f, 25.0f), FMath::FRandRange(-25.0f, 25.0f), FMath::FRandRange(0.0f, 20.0f));
	const FTransform Transform(FRotator::ZeroRotator, Location + Jitter);

	// 플레이어 피격: 빨강 / 치명타: 주황 / 일반: 흰색
	const FLinearColor Color = bVictimIsPlayer
		? FLinearColor(1.0f, 0.25f, 0.2f)
		: (bCritical ? FLinearColor(1.0f, 0.55f, 0.05f) : FLinearColor::White);

	AUT1DamageNumber* Number = World->SpawnActorDeferred<AUT1DamageNumber>(
		AUT1DamageNumber::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Number != nullptr)
	{
		Number->Setup(Damage, bCritical, Color);
		Number->FinishSpawning(Transform);
	}
}
