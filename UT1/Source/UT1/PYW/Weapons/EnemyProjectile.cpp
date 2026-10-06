#include "PYW/Weapons/EnemyProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PYW/Entities/EnemyCharacter.h"
#include "PYW/Combat/EnemyEffects.h"
#include "PYW/Combat/EnemyHitFlash.h"
#include "UObject/ConstructorHelpers.h"

AEnemyProjectile::AEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 5.0f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(14.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	RootComponent = Collision;

	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 떠 있는 작은 구체의 그림자가 바닥에 따로 찍혀 투사체가 둘로 보이지 않게 함
	Visual->SetCastShadow(false);
	// 기본 구체가 지름 100cm라서 충돌 구체(지름 28cm)와 비슷한 크기로 맞춤
	Visual->SetRelativeScale3D(FVector(0.3f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
	if (SphereMesh.Succeeded()) Visual->SetStaticMesh(SphereMesh.Object);

	Tail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tail"));
	Tail->SetupAttachment(Collision);
	Tail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Tail->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone"));
	if (ConeMesh.Succeeded()) Tail->SetStaticMesh(ConeMesh.Object);

	InnerTail = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InnerTail"));
	InnerTail->SetupAttachment(Collision);
	InnerTail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InnerTail->SetCastShadow(false);
	if (ConeMesh.Succeeded()) InnerTail->SetStaticMesh(ConeMesh.Object);

	Halo = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Halo"));
	Halo->SetupAttachment(Collision);
	Halo->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Halo->SetCastShadow(false);
	if (SphereMesh.Succeeded()) Halo->SetStaticMesh(SphereMesh.Object);

	// 그림자를 끈 작은 빛이라 비용이 낮음. 투사체가 바닥을 스치며 날아가는 것이 보이게 함
	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Collision);
	Glow->SetCastShadows(false);
	Glow->SetIntensityUnits(ELightUnits::Candelas);
	Glow->SetAttenuationRadius(320.0f);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->bRotationFollowsVelocity = true;
	Movement->ProjectileGravityScale = 0.0f;
	Collision->OnComponentHit.AddDynamic(this, &AEnemyProjectile::OnProjectileHit);
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AEnemyProjectile::OnProjectileOverlap);
}

void AEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Movement->InitialSpeed = Speed;
	// 포물선은 떨어지며 빨라지므로 속도 상한을 두지 않음 (0 = 제한 없음)
	Movement->MaxSpeed = GravityScale > 0.0f ? 0.0f : Speed;
	Movement->ProjectileGravityScale = GravityScale;
	if (bVisualOnly) Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement->Velocity = GetActorForwardVector() * Speed;
	if (CoreMaterial) Visual->SetMaterial(0, CoreMaterial);
	if (TailMaterial) Tail->SetMaterial(0, TailMaterial);
	if (InnerTailMaterial) InnerTail->SetMaterial(0, InnerTailMaterial);
	if (HaloMaterial) Halo->SetMaterial(0, HaloMaterial);
	const float Diameter = Collision->GetUnscaledSphereRadius() * 2.0f;
	LayoutTail(Tail, TailLength, Diameter * TailWidthScale);
	LayoutTail(InnerTail, InnerTailMaterial ? TailLength * InnerTailLengthScale : 0.0f, Diameter * InnerTailWidthScale);
	// 기본 구체는 지름 100cm임
	Halo->SetRelativeScale3D(FVector(Diameter * HaloScale / 100.0f));
	Halo->SetVisibility(HaloMaterial != nullptr && HaloScale > 0.0f);
	Glow->SetIntensity(GlowIntensity);
	Glow->SetLightColor(GlowColor);
	Glow->SetVisibility(GlowIntensity > 0.0f);
	EnemyEffects::SpawnAtLocation(this, LaunchEffect, GetActorLocation(), GetActorRotation(), LaunchEffectScale, LaunchDisabledEmitters);
	if (TrailEffect)
	{
		// 이펙트 수명을 액터에 묶어 두어 투사체가 사라질 때 함께 정리되게 함
		UNiagaraComponent* Trail = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailEffect, Collision, NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, TrailEffectScale, EAttachLocation::KeepRelativeOffset, true,
			ENCPoolMethod::None, false);
		EnemyEffects::ActivateWithDisabledEmitters(Trail, TrailDisabledEmitters);
	}
	UE_LOG(LogTemp, Display, TEXT("ENEMY_RANGED ProjectileLaunched Projectile=%s Location=%s Velocity=%s Trail=%s"),
		*GetName(), *GetActorLocation().ToCompactString(), *Movement->Velocity.ToCompactString(), *GetNameSafe(TrailEffect));
}

void AEnemyProjectile::LayoutTail(UStaticMeshComponent* Cone, float Length, float Diameter) const
{
	const UStaticMesh* Mesh = Cone->GetStaticMesh();
	if (Length <= 0.0f || Diameter <= 0.0f || !Mesh)
	{
		Cone->SetVisibility(false);
		return;
	}
	// 엔진 원뿔은 +Z가 꼭짓점임. Pitch 90으로 +Z를 -X(진행 반대)로 돌리고, 밑면이 코어 중심에 오게 밀어 둠
	const FBox Bounds = Mesh->GetBoundingBox();
	const float Height = FMath::Max(Bounds.Max.Z - Bounds.Min.Z, KINDA_SMALL_NUMBER);
	const float Width = FMath::Max(Bounds.Max.X - Bounds.Min.X, KINDA_SMALL_NUMBER);
	const float LengthScale = Length / Height;
	const float WidthScale = Diameter / Width;
	Cone->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	Cone->SetRelativeScale3D(FVector(WidthScale, WidthScale, LengthScale));
	Cone->SetRelativeLocation(FVector(Bounds.Min.Z * LengthScale, 0.0f, 0.0f));
}

void AEnemyProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	Explode(OtherActor, Hit.ImpactPoint);
}

void AEnemyProjectile::OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(OtherActor) || OtherActor == GetOwner() || OtherActor->IsA<AEnemyCharacter>()) return;
	Explode(OtherActor, GetActorLocation());
}

void AEnemyProjectile::Explode(AActor* OtherActor, const FVector& ImpactLocation)
{
	if (bExploded) return;
	bExploded = true;
	ApplyProjectileDamage(OtherActor);
	// 액터가 바로 제거되므로 붙이지 않고 월드 위치에 독립적으로 생성함
	EnemyEffects::SpawnAtLocation(this, ImpactEffect, ImpactLocation, GetActorRotation(), ImpactEffectScale, ImpactDisabledEmitters);
	// 맞은 자리에서 투사체와 같은 색의 섬광이 터짐. 대상(플레이어)에 맞으면 더 크게 터뜨려 맞았다는 걸 분명히 함
	const bool bHitPawn = IsValid(OtherActor) && OtherActor->IsA<APawn>();
	AEnemyHitFlash::Spawn(GetWorld(), ImpactLocation, GlowColor * 1.5f, bHitPawn ? ImpactFlashRadius * 1.4f : ImpactFlashRadius);
	UE_LOG(LogTemp, Display, TEXT("ENEMY_RANGED ProjectileExploded Projectile=%s Location=%s Impact=%s"),
		*GetName(), *ImpactLocation.ToCompactString(), *GetNameSafe(ImpactEffect));
	Destroy();
}

void AEnemyProjectile::ApplyProjectileDamage(AActor* OtherActor)
{
	if (!IsValid(OtherActor) || OtherActor == GetOwner()) return;
	UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, UDamageType::StaticClass());
	UE_LOG(LogTemp, Display, TEXT("ENEMY_RANGED ProjectileHit Target=%s Damage=%.1f"), *GetNameSafe(OtherActor), Damage);
}
