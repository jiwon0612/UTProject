#include "PYW/Gameplay/EnemyProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PYW/Entities/EnemyCharacter.h"
#include "PYW/Gameplay/EnemyEffects.h"
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
	Movement->MaxSpeed = Speed;
	Movement->Velocity = GetActorForwardVector() * Speed;
	if (CoreMaterial) Visual->SetMaterial(0, CoreMaterial);
	if (TailMaterial) Tail->SetMaterial(0, TailMaterial);
	LayoutTail();
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

void AEnemyProjectile::LayoutTail()
{
	const UStaticMesh* Mesh = Tail->GetStaticMesh();
	if (TailLength <= 0.0f || !Mesh)
	{
		Tail->SetVisibility(false);
		return;
	}
	// 엔진 원뿔은 +Z가 꼭짓점임. Pitch 90으로 +Z를 -X(진행 반대)로 돌리고, 밑면이 코어 중심에 오게 밀어 둠
	const FBox Bounds = Mesh->GetBoundingBox();
	const float Height = FMath::Max(Bounds.Max.Z - Bounds.Min.Z, KINDA_SMALL_NUMBER);
	const float Width = FMath::Max(Bounds.Max.X - Bounds.Min.X, KINDA_SMALL_NUMBER);
	const float LengthScale = TailLength / Height;
	const float WidthScale = Collision->GetUnscaledSphereRadius() * 2.0f / Width;
	Tail->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	Tail->SetRelativeScale3D(FVector(WidthScale, WidthScale, LengthScale));
	Tail->SetRelativeLocation(FVector(Bounds.Min.Z * LengthScale, 0.0f, 0.0f));
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
