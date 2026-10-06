// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UT1LootDropComponent.h"
#include "CJW/Crafting/UT1LootPickup.h"
#include "CJW/Crafting/UT1RunInventoryComponent.h"
#include "CJW/Entities/UT1Entity.h"
#include "CJW/Player/UT1Player.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UT1.h"

UUT1LootDropComponent::UUT1LootDropComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	PickupClass = AUT1LootPickup::StaticClass();
}

void UUT1LootDropComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bDropOnOwnerDeath)
	{
		if (AUT1Entity* Entity = Cast<AUT1Entity>(GetOwner()))
		{
			Entity->OnDied.AddDynamic(this, &UUT1LootDropComponent::HandleOwnerDied);
		}
	}
}

void UUT1LootDropComponent::HandleOwnerDied(AUT1Entity* Entity)
{
	DropLoot();
}

void UUT1LootDropComponent::DropLoot()
{
	UWorld* World = GetWorld();
	if (World == nullptr || PickupClass == nullptr)
	{
		return;
	}

	// --- 무엇을 떨굴지 먼저 정한다 ---
	struct FPendingDrop
	{
		UUT1MaterialData* Material = nullptr;
		int32 Count = 0;
		UUT1WeaponData* Blueprint = nullptr;
	};
	TArray<FPendingDrop> Pending;

	for (const FUT1MaterialDrop& Drop : MaterialDrops)
	{
		if (Drop.Material != nullptr && FMath::FRand() < Drop.Chance)
		{
			FPendingDrop& Entry = Pending.AddDefaulted_GetRef();
			Entry.Material = Drop.Material;
			Entry.Count = FMath::RandRange(Drop.MinCount, FMath::Max(Drop.MinCount, Drop.MaxCount));
		}
	}

	// 싱글 플레이 기준으로 0번 플레이어의 해금 상태를 본다.
	const AUT1Player* Player = Cast<AUT1Player>(UGameplayStatics::GetPlayerPawn(this, 0));
	const UUT1RunInventoryComponent* Inventory = Player != nullptr ? Player->GetRunInventory() : nullptr;

	for (const FUT1BlueprintDrop& Drop : BlueprintDrops)
	{
		if (Drop.Weapon == nullptr || (Inventory != nullptr && Inventory->HasBlueprint(Drop.Weapon)))
		{
			continue;
		}
		if (FMath::FRand() < Drop.Chance)
		{
			FPendingDrop& Entry = Pending.AddDefaulted_GetRef();
			Entry.Blueprint = Drop.Weapon;
			break;
		}
	}

	if (Pending.Num() == 0)
	{
		return;
	}

	// --- 주인 주변 원 위에 고르게 놓는다 ---
	const FVector Origin = FindDropOrigin();
	const float StartAngle = FMath::FRandRange(0.0f, 360.0f);

	for (int32 i = 0; i < Pending.Num(); ++i)
	{
		const float Angle = FMath::DegreesToRadians(StartAngle + 360.0f * i / Pending.Num());
		const float Radius = Pending.Num() == 1 ? 0.0f : ScatterRadius;
		const FVector Location = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Radius;

		const FPendingDrop& Drop = Pending[i];
		SpawnPickup(Location, [&Drop](AUT1LootPickup* Pickup)
		{
			if (Drop.Blueprint != nullptr)
			{
				Pickup->InitBlueprint(Drop.Blueprint);
			}
			else
			{
				Pickup->InitMaterial(Drop.Material, Drop.Count);
			}
		});
	}
}

FVector UUT1LootDropComponent::FindDropOrigin() const
{
	const AActor* Owner = GetOwner();
	const FVector Start = Owner->GetActorLocation();

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UT1LootDrop), false, Owner);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start - FVector(0.0f, 0.0f, 1000.0f), ECC_Visibility, Params))
	{
		// 픽업 판정 구(반지름 60)가 바닥에 반쯤 묻히도록 조금만 띄운다.
		return Hit.ImpactPoint + FVector(0.0f, 0.0f, 30.0f);
	}
	return Start;
}

AUT1LootPickup* UUT1LootDropComponent::SpawnPickup(const FVector& Location, TFunctionRef<void(AUT1LootPickup*)> Init)
{
	// 지연 스폰: 생성 -> 내용물 채우기 -> FinishSpawning 순서.
	// 일반 SpawnActor 는 스폰 순간 겹침 판정을 해서, 플레이어 바로 옆에 떨어지면
	// 내용물을 채우기도 전에 빈 픽업이 주워질 수 있다.
	const FTransform Transform(FRotator::ZeroRotator, Location);
	AUT1LootPickup* Pickup = GetWorld()->SpawnActorDeferred<AUT1LootPickup>(
		PickupClass, Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Pickup == nullptr)
	{
		return nullptr;
	}

	Init(Pickup);
	Pickup->FinishSpawning(Transform);
	return Pickup;
}
