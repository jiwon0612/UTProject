#include "LSW/Rooms/UT1_RewardChest.h"

#include "CJW/Crafting/UT1LootPickup.h"
#include "CJW/Crafting/UT1MaterialData.h"
#include "CJW/Player/UT1Player.h"
#include "CJW/Weapons/UT1WeaponData.h"
#include "LSW/Rooms/UT1_RoomManager.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AUT1_RewardChest::AUT1_RewardChest()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	InteractRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractRange"));
	SetRootComponent(InteractRange);
	InteractRange->SetSphereRadius(220.0f);
	InteractRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(InteractRange);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ChestAsset(
		TEXT("/Game/CJW/Assets/Dungeon_Pack/Assets/Models/SM_Metal_Chest.SM_Metal_Chest"));
	if (ChestAsset.Succeeded())
	{
		BodyMesh->SetStaticMesh(ChestAsset.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ChestFadeAsset(
		TEXT("/Game/LSW/Materials/M_UT1_RewardChestFade.M_UT1_RewardChestFade"));
	if (ChestFadeAsset.Succeeded())
	{
		ChestFadeMaterial = ChestFadeAsset.Object;
	}

	PromptText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PromptText"));
	PromptText->SetupAttachment(InteractRange);
	PromptText->SetText(NSLOCTEXT("UT1RewardChest", "Prompt", "E: 상자 열기"));
	PromptText->SetHorizontalAlignment(EHTA_Center);
	PromptText->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
	PromptText->SetRelativeRotation(FRotator(50.0f, 225.0f, 0.0f));
	PromptText->SetHiddenInGame(true);

	PickupClass = AUT1LootPickup::StaticClass();
}

void AUT1_RewardChest::BeginPlay()
{
	Super::BeginPlay();

	// The Blueprint previously stored the dissolve material as its default, which
	// hid parts of the closed chest before interaction. Restore the mesh's asset materials.
	const UStaticMesh* ChestMesh = BodyMesh ? BodyMesh->GetStaticMesh() : nullptr;
	if (!ChestMesh || !BodyMesh)
	{
		return;
	}

	for (int32 MaterialIndex = 0; MaterialIndex < BodyMesh->GetNumMaterials(); ++MaterialIndex)
	{
		if (UMaterialInterface* OriginalMaterial = ChestMesh->GetMaterial(MaterialIndex))
		{
			BodyMesh->SetMaterial(MaterialIndex, OriginalMaterial);
		}
	}
}

void AUT1_RewardChest::Interact_Implementation(AUT1Player* Interactor)
{
	if (bOpened || Interactor == nullptr || Interactor->IsDead())
	{
		return;
	}

	TArray<int32> ValidRewardIndices;
	for (int32 Index = 0; Index < Rewards.Num(); ++Index)
	{
		const FUT1RewardChestEntry& Reward = Rewards[Index];
		const bool bHasMaterial = Reward.Material != nullptr;
		const bool bHasBlueprint = Reward.Blueprint != nullptr;
		if (bHasMaterial != bHasBlueprint)
		{
			ValidRewardIndices.Add(Index);
		}
	}

	if (ValidRewardIndices.IsEmpty() || PickupClass == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	const int32 RewardCount = FMath::RandRange(
		FMath::Max(1, MinRewardCount),
		FMath::Max(FMath::Max(1, MinRewardCount), MaxRewardCount));
	const float StartAngle = FMath::FRandRange(0.0f, 360.0f);
	const float ScatterRadius = FMath::Max(140.0f, RewardCount * 45.0f);
	int32 SpawnedRewardCount = 0;

	for (int32 Index = 0; Index < RewardCount; ++Index)
	{
		const FUT1RewardChestEntry& Reward = Rewards[
			ValidRewardIndices[FMath::RandRange(0, ValidRewardIndices.Num() - 1)]];
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float Radius = FMath::FRandRange(ScatterRadius * 0.65f, ScatterRadius);
		const FVector ScatterOffset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
		FVector EndLocation = GetActorLocation() + ScatterOffset;

		FHitResult FloorHit;
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(RewardChestLanding), false, this);
		TraceParams.AddIgnoredActor(Interactor);
		const FVector TraceStart = EndLocation + FVector(0.0f, 0.0f, 1000.0f);
		const FVector TraceEnd = EndLocation - FVector(0.0f, 0.0f, 1000.0f);
		if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, TraceParams))
		{
			EndLocation = FloorHit.ImpactPoint + FVector(0.0f, 0.0f, 35.0f);
		}
		else
		{
			EndLocation.Z = GetActorLocation().Z + 35.0f;
		}

		const FVector StartLocation = GetActorLocation() + FVector(0.0f, 0.0f, 150.0f)
			+ FVector(FMath::FRandRange(-20.0f, 20.0f), FMath::FRandRange(-20.0f, 20.0f), 0.0f);
		const FTransform SpawnTransform(GetActorRotation(), StartLocation);

		AUT1LootPickup* Pickup = GetWorld()->SpawnActorDeferred<AUT1LootPickup>(
			PickupClass,
			SpawnTransform,
			nullptr,
			Interactor,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Pickup == nullptr)
		{
			continue;
		}

		if (Reward.Material != nullptr)
		{
			Pickup->InitMaterial(Reward.Material, Reward.Count);
		}
		else
		{
			Pickup->InitBlueprint(Reward.Blueprint);
		}

		Pickup->SetActorEnableCollision(false);
		Pickup->FinishSpawning(SpawnTransform);
		Pickup->SetActorEnableCollision(false);

		FUT1RewardPickupFlight& Flight = TossingPickups.AddDefaulted_GetRef();
		Flight.Pickup = Pickup;
		Flight.StartLocation = StartLocation;
		Flight.EndLocation = EndLocation;
		Flight.Duration = FMath::FRandRange(0.65f, 1.0f);
		Flight.ArcHeight = FMath::FRandRange(160.0f, 280.0f);
		++SpawnedRewardCount;
	}

	if (SpawnedRewardCount == 0)
	{
		return;
	}

	bOpened = true;
	PromptText->SetHiddenInGame(true);
	SetActorTickEnabled(true);
	StartChestFade();

	if (AUT1_RoomManager* RoomManager = Cast<AUT1_RoomManager>(
		UGameplayStatics::GetActorOfClass(GetWorld(), AUT1_RoomManager::StaticClass())))
	{
		RoomManager->MarkCurrentRoomCleared();
	}
}

void AUT1_RewardChest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (int32 Index = TossingPickups.Num() - 1; Index >= 0; --Index)
	{
		FUT1RewardPickupFlight& Flight = TossingPickups[Index];
		AUT1LootPickup* Pickup = Flight.Pickup.Get();
		if (!IsValid(Pickup))
		{
			TossingPickups.RemoveAtSwap(Index);
			continue;
		}

		Flight.Elapsed += DeltaSeconds;
		const float Progress = FMath::Clamp(Flight.Elapsed / Flight.Duration, 0.0f, 1.0f);
		const FVector BaseLocation = FMath::Lerp(Flight.StartLocation, Flight.EndLocation, Progress);
		const float ArcOffset = FMath::Sin(Progress * PI) * Flight.ArcHeight;
		Pickup->SetActorLocation(BaseLocation + FVector(0.0f, 0.0f, ArcOffset));

		if (Progress >= 1.0f)
		{
			Pickup->SetActorLocation(Flight.EndLocation);
			Pickup->SetActorEnableCollision(true);
			TossingPickups.RemoveAtSwap(Index);
		}
	}

	if (bFadingOut)
	{
		UpdateChestFade(DeltaSeconds);
	}

	if (TossingPickups.IsEmpty() && !bFadingOut)
	{
		SetActorTickEnabled(false);
	}
}

void AUT1_RewardChest::StartChestFade()
{
	FadeMaterials.Reset();
	if (ChestFadeMaterial)
	{
		for (int32 MaterialIndex = 0; MaterialIndex < BodyMesh->GetNumMaterials(); ++MaterialIndex)
		{
			BodyMesh->SetMaterial(MaterialIndex, ChestFadeMaterial);
		}
	}

	for (int32 MaterialIndex = 0; MaterialIndex < BodyMesh->GetNumMaterials(); ++MaterialIndex)
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = BodyMesh->CreateAndSetMaterialInstanceDynamic(MaterialIndex))
		{
			DynamicMaterial->SetScalarParameterValue(FadeOpacityParameter, 1.0f);
			FadeMaterials.Add(DynamicMaterial);
		}
	}

	FadeElapsed = 0.0f;
	bFadingOut = FadeMaterials.Num() > 0;
	if (!bFadingOut)
	{
		Destroy();
	}
}

void AUT1_RewardChest::UpdateChestFade(float DeltaSeconds)
{
	FadeElapsed += DeltaSeconds;
	const float Progress = FMath::Clamp(FadeElapsed / FadeDuration, 0.0f, 1.0f);
	const float Opacity = 1.0f - Progress;
	for (UMaterialInstanceDynamic* DynamicMaterial : FadeMaterials)
	{
		if (IsValid(DynamicMaterial))
		{
			DynamicMaterial->SetScalarParameterValue(FadeOpacityParameter, Opacity);
		}
	}

	if (Progress >= 1.0f)
	{
		bFadingOut = false;
		Destroy();
	}
}

void AUT1_RewardChest::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	if (!bOpened && Cast<AUT1Player>(OtherActor) != nullptr)
	{
		PromptText->SetHiddenInGame(false);
	}
}

void AUT1_RewardChest::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);
	if (Cast<AUT1Player>(OtherActor) != nullptr)
	{
		PromptText->SetHiddenInGame(true);
	}
}
