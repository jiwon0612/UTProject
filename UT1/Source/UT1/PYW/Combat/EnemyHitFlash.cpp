#include "PYW/Combat/EnemyHitFlash.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// PYW 가산 머티리얼임 (Color 파라미터로 색과 밝기를 바꿈)
	const TCHAR* FlashMaterialPath = TEXT("/Game/PYW/Materials/M_EnemyHitFlash.M_EnemyHitFlash");
}

AEnemyHitFlash::AEnemyHitFlash()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	auto MakeMesh = [this, Root](const TCHAR* Name, const TCHAR* MeshPath)
	{
		UStaticMeshComponent* Mesh = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Mesh->SetupAttachment(Root);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCastShadow(false);
		ConstructorHelpers::FObjectFinder<UStaticMesh> Asset(MeshPath);
		if (Asset.Succeeded()) Mesh->SetStaticMesh(Asset.Object);
		return Mesh;
	};
	Burst = MakeMesh(TEXT("Burst"), TEXT("/Engine/BasicShapes/Sphere"));
	Ring = MakeMesh(TEXT("Ring"), TEXT("/Game/PYW/ProjectileVFX/Meshes/SM_Torus"));

	Flash = CreateDefaultSubobject<UPointLightComponent>(TEXT("Flash"));
	Flash->SetupAttachment(Root);
	Flash->SetCastShadows(false);
	Flash->SetIntensityUnits(ELightUnits::Candelas);
}

AEnemyHitFlash* AEnemyHitFlash::Spawn(UWorld* World, const FVector& Location, const FLinearColor& Color, float Radius)
{
	if (!World) return nullptr;
	const FTransform Transform(Location);
	AEnemyHitFlash* HitFlash = World->SpawnActorDeferred<AEnemyHitFlash>(AEnemyHitFlash::StaticClass(), Transform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!HitFlash) return nullptr;
	HitFlash->Color = Color;
	HitFlash->Radius = Radius;
	HitFlash->FinishSpawning(Transform);
	return HitFlash;
}

void AEnemyHitFlash::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, FlashMaterialPath))
	{
		BurstMaterial = Burst->CreateDynamicMaterialInstance(0, Material);
		RingMaterial = Ring->CreateDynamicMaterialInstance(0, Material);
	}
	if (const UStaticMesh* RingMesh = Ring->GetStaticMesh())
	{
		const FVector Size = RingMesh->GetBoundingBox().GetSize();
		RingMeshSize = static_cast<float>(FMath::Max3(Size.X, Size.Y, 1.0));
	}
	Flash->SetLightColor(Color);
	Flash->SetAttenuationRadius(Radius * 5.0f);
	UpdateFlash(0.0f);
}

void AEnemyHitFlash::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float Alpha = Age / Lifetime;
	if (Alpha >= 1.0f)
	{
		Destroy();
		return;
	}
	UpdateFlash(Alpha);
}

void AEnemyHitFlash::UpdateFlash(float Alpha)
{
	// 처음에 빠르게 커지고(ease-out) 밝기는 제곱으로 빠르게 꺼져서 "탁" 터지는 느낌을 줌
	const float Grow = 1.0f - FMath::Square(1.0f - Alpha);
	const float Fade = FMath::Square(1.0f - Alpha);

	// 기본 구체는 지름 100cm임
	Burst->SetRelativeScale3D(FVector(Radius * 2.0f / 100.0f * FMath::Lerp(0.35f, 1.0f, Grow)));
	// 고리는 구체보다 넓게 퍼져서 맞은 지점을 바닥과 나란히 표시함
	Ring->SetRelativeScale3D(FVector(Radius * 2.0f / RingMeshSize * FMath::Lerp(0.5f, 1.8f, Grow)));

	const FLinearColor Tint = Color * PeakIntensity * Fade;
	if (BurstMaterial) BurstMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
	if (RingMaterial) RingMaterial->SetVectorParameterValue(TEXT("Color"), Tint * 0.6f);
	Flash->SetIntensity(PeakLightIntensity * Fade);
}
