// Fill out your copyright notice in the Description page of Project Settings.


#include "CJW/Crafting/UT1LootPickup.h"
#include "CJW/Crafting/UT1RunInventoryComponent.h"
#include "CJW/Crafting/UT1MaterialData.h"
#include "CJW/Weapons/UT1WeaponData.h"
#include "CJW/Player/UT1Player.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"
#include "UT1.h"

#define LOCTEXT_NAMESPACE "UT1Loot"

AUT1LootPickup::AUT1LootPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupRange = CreateDefaultSubobject<USphereComponent>(TEXT("PickupRange"));
	PickupRange->SetSphereRadius(60.0f);
	PickupRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetRootComponent(PickupRange);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(PickupRange);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.3f));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(PickupRange);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(18.0f);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	// 작업대 안내 문구와 같은 이유로 카메라 쪽을 보게 한다 (UT1Workbench.cpp 참고).
	Label->SetRelativeRotation(FRotator(50.0f, 225.0f, 0.0f));

	// 엔진 기본 도형은 어느 프로젝트에나 있어서 에셋 없이도 바로 보인다.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	MaterialShape = SphereAsset.Object;
	BlueprintShape = CubeAsset.Object;
	Mesh->SetStaticMesh(MaterialShape);
}

void AUT1LootPickup::InitMaterial(UUT1MaterialData* InMaterial, int32 InCount)
{
	Material = InMaterial;
	Count = FMath::Max(InCount, 1);
	BlueprintWeapon = nullptr;
	RefreshVisual();
}

void AUT1LootPickup::InitBlueprint(UUT1WeaponData* InWeapon)
{
	BlueprintWeapon = InWeapon;
	Material = nullptr;
	RefreshVisual();
}

void AUT1LootPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 레벨에 직접 배치하고 디테일 패널에서 내용물을 바꿨을 때도 라벨이 따라오게 한다.
	RefreshVisual();
}

void AUT1LootPickup::RefreshVisual()
{
	if (BlueprintWeapon != nullptr)
	{
		Label->SetText(FText::Format(LOCTEXT("BlueprintLabel", "설계도: {0}"), BlueprintWeapon->GetDisplayText()));
		Label->SetTextRenderColor(FColor(255, 200, 60));
		if (BlueprintShape != nullptr && Mesh->GetStaticMesh() == MaterialShape)
		{
			Mesh->SetStaticMesh(BlueprintShape);
		}
	}
	else if (Material != nullptr)
	{
		Label->SetText(FText::Format(LOCTEXT("MaterialLabel", "{0} x{1}"), Material->GetDisplayText(), Count));
		Label->SetTextRenderColor(FColor::White);
		// 기본 도형끼리만 바꾼다. BP 에서 지정한 메시는 건드리지 않는다.
		if (MaterialShape != nullptr && Mesh->GetStaticMesh() == BlueprintShape)
		{
			Mesh->SetStaticMesh(MaterialShape);
		}
	}
	else
	{
		Label->SetText(FText::GetEmpty());
	}
}

void AUT1LootPickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (bCollected)
	{
		return;
	}

	AUT1Player* Player = Cast<AUT1Player>(OtherActor);
	if (Player == nullptr || Player->IsDead())
	{
		return;
	}

	UUT1RunInventoryComponent* Inventory = Player->GetRunInventory();
	if (Inventory == nullptr)
	{
		return;
	}

	FText Message;
	if (BlueprintWeapon != nullptr)
	{
		// 이미 가진 설계도여도 주운 것으로 치고 사라진다. 드랍 쪽에서 가진 설계도는
		// 굴리지 않으므로 보통은 일어나지 않고, 레벨에 직접 놓은 경우만 해당한다.
		const bool bNew = Inventory->UnlockBlueprint(BlueprintWeapon);
		Message = FText::Format(bNew
			? LOCTEXT("GotBlueprint", "설계도 획득: {0}")
			: LOCTEXT("DupBlueprint", "이미 가진 설계도: {0}"), BlueprintWeapon->GetDisplayText());
	}
	else if (Material != nullptr)
	{
		Inventory->AddMaterial(Material, Count);
		Message = FText::Format(LOCTEXT("GotMaterial", "{0} x{1} 획득"), Material->GetDisplayText(), Count);
	}
	else
	{
		return;   // 내용물이 없는 픽업은 무시한다 (배치 실수).
	}

	bCollected = true;

	// 획득 알림. 지금은 디버그 메시지로 대신하고, HUD 알림 위젯은 UI 를 꾸밀 때 만든다.
	UE_LOG(LogUT1, Log, TEXT("[Loot] %s"), *Message.ToString());
	if (GEngine != nullptr)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, BlueprintWeapon != nullptr ? FColor::Yellow : FColor::Green, Message.ToString());
	}

	Destroy();
}

#undef LOCTEXT_NAMESPACE
