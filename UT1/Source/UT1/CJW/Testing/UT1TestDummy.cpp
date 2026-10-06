// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Testing/UT1TestDummy.h"
#include "CJW/Crafting/UT1LootDropComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "UT1.h"

AUT1TestDummy::AUT1TestDummy()
{
	PrimaryActorTick.bCanEverTick = false;

	// 기본 공격 10 데미지 기준 다섯 대면 죽는다. 사망까지 금방 확인된다.
	MaxHealth = 50.0f;

	LootDrop = CreateDefaultSubobject<UUT1LootDropComponent>(TEXT("LootDrop"));

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(GetCapsuleComponent());
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeAsset.Object);
		VisualMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.9f));
	}
}

void AUT1TestDummy::HandleDamaged(float ActualDamage, AActor* DamageCauser)
{
	Super::HandleDamaged(ActualDamage, DamageCauser);

	const FString CauserName = (DamageCauser != nullptr) ? DamageCauser->GetName() : TEXT("None");
	UE_LOG(LogUT1, Warning, TEXT("[Dummy] -%.1f  체력 %.1f/%.1f  가해자=%s"),
		ActualDamage, GetHealth(), GetMaxHealth(), *CauserName);

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
			FString::Printf(TEXT("Dummy  -%.0f    HP %.0f / %.0f"),
				ActualDamage, GetHealth(), GetMaxHealth()));
	}
}

void AUT1TestDummy::HandleDeath(AActor* Killer)
{
	Super::HandleDeath(Killer);

	UE_LOG(LogUT1, Warning, TEXT("[Dummy] 사망"));

	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("Dummy DEAD"));
	}

	// 사망 몽타주가 없으니 쓰러뜨려서 눈으로 알 수 있게 한다.
	if (VisualMesh != nullptr)
	{
		VisualMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	}
}
