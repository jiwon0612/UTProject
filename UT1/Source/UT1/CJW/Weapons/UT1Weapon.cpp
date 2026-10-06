// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Weapons/UT1Weapon.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "UT1.h"

AUT1Weapon::AUT1Weapon()
{
	PrimaryActorTick.bCanEverTick = false;

	// 빈 씬 컴포넌트를 루트로 둬야 그 아래에서 메시를 옮길 수 있다.
	// 메시 자신이 루트면 자기 기준으로 오프셋을 줄 수 없다.
	GripRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GripRoot"));
	SetRootComponent(GripRoot);

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GripRoot);

	// 손에 들린 무기가 캐릭터를 밀거나 벽에 걸리면 안 된다.
	// 타격 판정은 나중에 메시 소켓 사이를 잇는 트레이스로 따로 처리한다.
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);

	// 날의 양 끝. 기본 위치는 의미가 없고 무기 BP 에서 맞춘다.
	TraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("TraceStart"));
	TraceStart->SetupAttachment(WeaponMesh);

	TraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("TraceEnd"));
	TraceEnd->SetupAttachment(WeaponMesh);
}

void AUT1Weapon::BeginAttackTrace()
{
	HitActorsThisSwing.Reset();

	// 시작 프레임의 위치를 기준으로 잡아 둔다. 이게 없으면 첫 Tick 이
	// 엉뚱한 과거 위치에서부터 쓸어서 뒤에 있던 적까지 때린다.
	BuildSamplePoints(PreviousSamplePoints);
	bTracing = true;
}

void AUT1Weapon::TickAttackTrace(float Damage)
{
	if (bTracing == false)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || TraceStart == nullptr || TraceEnd == nullptr)
	{
		return;
	}

	TArray<FVector> Current;
	BuildSamplePoints(Current);
	if (Current.Num() == 0 || Current.Num() != PreviousSamplePoints.Num())
	{
		PreviousSamplePoints = MoveTemp(Current);
		return;
	}

	AActor* MyOwner = GetOwner();

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UT1WeaponTrace), false, this);
	QueryParams.AddIgnoredActor(this);
	if (MyOwner != nullptr)
	{
		QueryParams.AddIgnoredActor(MyOwner);   // 자기 자신을 베지 않도록
	}

	for (int32 i = 0; i < Current.Num(); ++i)
	{
		TArray<FHitResult> Hits;
		World->SweepMultiByChannel(
			Hits,
			PreviousSamplePoints[i],
			Current[i],
			FQuat::Identity,
			UT1_TRACE_CHANNEL_WEAPON,
			FCollisionShape::MakeSphere(TraceRadius),
			QueryParams);

		for (const FHitResult& Hit : Hits)
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor == nullptr || HitActorsThisSwing.Contains(HitActor))
			{
				continue;   // 한 스윙에 한 번만
			}
			HitActorsThisSwing.Add(HitActor);

			// 체력 시스템이 아직 없어서 엔진 표준 경로를 쓴다.
			// 맞는 쪽이 AActor::TakeDamage 를 오버라이드하면 받을 수 있고,
			// 나중에 전용 체력 컴포넌트가 생기면 이 호출만 바꾸면 된다.
			UGameplayStatics::ApplyPointDamage(
				HitActor,
				Damage,
				(Current[i] - PreviousSamplePoints[i]).GetSafeNormal(),
				Hit,
				GetInstigatorController(),
				MyOwner != nullptr ? MyOwner : this,
				nullptr);
		}

		if (bDrawDebugTrace)
		{
			DrawDebugLine(World, PreviousSamplePoints[i], Current[i],
				Hits.Num() > 0 ? FColor::Red : FColor::Green, false, 1.0f, 0, 0.5f);
		}
	}

	PreviousSamplePoints = MoveTemp(Current);
}

void AUT1Weapon::EndAttackTrace()
{
	bTracing = false;
	PreviousSamplePoints.Reset();
	HitActorsThisSwing.Reset();
}

void AUT1Weapon::BuildSamplePoints(TArray<FVector>& Out) const
{
	Out.Reset();
	if (TraceStart == nullptr || TraceEnd == nullptr)
	{
		return;
	}

	const FVector A = TraceStart->GetComponentLocation();
	const FVector B = TraceEnd->GetComponentLocation();
	const int32 Count = FMath::Max(2, TraceSampleCount);

	Out.Reserve(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		Out.Add(FMath::Lerp(A, B, static_cast<float>(i) / static_cast<float>(Count - 1)));
	}
}
